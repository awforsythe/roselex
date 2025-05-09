#include "rl_board.h"

#include <cstdlib>
#include <cassert>
#include <cstring>

#include "rl_util.h"
#include "rl_dawg.h"
#include "rl_node.h"
#include "rl_edgemap.h"

static void _rl_board_clear(rl_board& board)
{
	// Initialize all cells to hold a letter value of RL_BLANK
	const int32 num_squares = board.size_x * board.size_y;
	memset(board.letters, RL_BLANK, num_squares);

	// Initialize blockflags row-by-row, making the top and bottom edge blocked at the extents of the board
	memset(board.blockflags, RL_BLOCKFLAG_PREV_DOWN, board.size_x);
	memset(board.blockflags + board.size_x, 0, board.size_x * (board.size_y - 2));
	memset(board.blockflags + board.size_x * (board.size_y - 1), RL_BLOCKFLAG_NEXT_DOWN, board.size_x);

	// Additionally, mark the first and last column as blocked at the left and right edges
	for (int32 y = 0; y < board.size_y; y++)
	{
		const int32 row_start = board.size_x * y;
		board.blockflags[row_start] |= RL_BLOCKFLAG_PREV_ACROSS;
		board.blockflags[row_start + board.size_x - 1] |= RL_BLOCKFLAG_NEXT_ACROSS;
	}

	// Initialize the checkbits for all squares to allow any combination of letters
	for (int32 i = 0; i < num_squares; i++)
	{
		board.checkbits_x[i] = RL_CHECKBITS_ANY;
		board.checkbits_y[i] = RL_CHECKBITS_ANY;
	}
}

static int32 _rl_board_flag_dirty_anchor(rl_board& board, int32 from_index, int32 search_dir_offset, uint8 blockflag, int32* index_array, int32 index_array_size, int32 index_array_capacity)
{
	// From our starting cell, keep iterating in the search direction until we're blocked (and abort) or we find a blank cell (and flag it as an anchor that needs updating)
	int32 index = from_index;
	while (true)
	{
		// If we're blocked from reaching the next cell at any point before we find a blank, abort: there are no anchors to update in this direction
		const bool next_is_blocked = (board.blockflags[index] & blockflag) != 0;
		if (next_is_blocked)
		{
			return index_array_size;
		}

		// If the next cell in our search direction is a blank or an anchor, it's been affected by our move and should be flagged as dirty so its checkbits can be recomputed
		const int32 next_index = index + search_dir_offset;
		if (board.letters[next_index] == RL_BLANK || board.letters[next_index] == RL_ANCHOR)
		{
			// Flag the square as an anchor
			board.letters[next_index] = RL_ANCHOR;

			// Push the index of that square into the array we've been given, and return the upated array size
			assert(index_array_size < index_array_capacity);
			index_array[index_array_size] = next_index;
			return index_array_size + 1;
		}

		// Otherwise, the next cell should be a letter connected to the word we're playing: continue our search from that position
		assert(board.letters[next_index] >= 'a' && board.letters[next_index] <= 'z');
		index = next_index;
	}
}

static bool _rl_board_is_letter(const rl_board& board, int32 index)
{
	assert(index >= 0 && index < board.size_x * board.size_y);

	const uint8 letter_val = board.letters[index];
	if (letter_val != RL_BLANK && letter_val != RL_ANCHOR)
	{
		assert(letter_val >= 'a' && letter_val <= 'z');
		return true;
	}
	return false;
}

static bool _rl_board_check_suffix(const rl_dawg& dawg, int32 node_index, const rl_board& board, int32 anchor_index, int32 offset, int32 suffix_len)
{
	// Iterate through the letters of the suffix on the board, starting from the right of the anchor
	for (int32 depth = 1; depth <= suffix_len; depth++)
	{
		// Find the letter at this square within the suffix
		const int32 suffix_letter_index = anchor_index + (offset * depth);
		assert(_rl_board_is_letter(board, suffix_letter_index));
		const uint8 letter = board.letters[suffix_letter_index];

		// Find an edge from the current DAWG node labeled with that letter
		const rl_node& node = dawg.nodearray.items[node_index];
		node_index = rl_edgemap_find(node.next_by_letter, letter);

		// If there's no such letter, then we can't build a word off of the prefix node using this suffix
		if (node_index < 0)
		{
			return false;
		}
	}

	// The suffix works if the node that we've arrived at (or the prefix itself, in the case of no suffix) forms a valid word
	return dawg.nodearray.items[node_index].is_word;
}

static uint32 _rl_board_resolve_checkbits(const rl_dawg& dawg, const rl_board& board, int32 anchor_index, int32 offset, uint8 blockflag_prev, uint8 blockflag_next)
{
	// Search in either direction above and below our anchor to figure out the length of any existing words adjacent to it
	int32 prefix_len = 0;
	{
		int32 index = anchor_index;
		while ((board.blockflags[index] & blockflag_prev) == 0)
		{
			const int32 prev_index = index - offset;
			if (!_rl_board_is_letter(board, prev_index))
			{
				break;
			}
			prefix_len++;
			index = prev_index;
		}
	}

	int32 suffix_len = 0;
	{
		int32 index = anchor_index;
		while ((board.blockflags[index] & blockflag_next) == 0)
		{
			const int32 next_index = index + offset;
			if (!_rl_board_is_letter(board, next_index))
			{
				break;
			}
			suffix_len++;
			index = next_index;
		}
	}

	// If we have a prefix or a suffix, we need to do a constrained DAWG search for valid words; otherwise any letter is valid here
	if (prefix_len > 0 || suffix_len > 0)
	{
		// Establish our resulting bit vector, with bits representing 'a' thru 'z'
		uint32 value = 0;

		// If there's a prefix, traverse the DAWG to find the corresponding node (start from the root if no prefix)
		int32 prefix_node_index = 0;
		for (int32 index = anchor_index - (offset * prefix_len); index < anchor_index; index += offset)
		{
			assert(_rl_board_is_letter(board, index));
			const uint8 letter = board.letters[index];

			// If the prefix is not contained in the DAWG, no letters can be played here
			const rl_node& node = dawg.nodearray.items[prefix_node_index];
			const int32 next_node_index = rl_edgemap_find(node.next_by_letter, letter);
			if (next_node_index < 0)
			{
				return 0;
			}
			prefix_node_index = next_node_index;
		}

		// From that node, check each edge (labeled with letter L) leading out of that node to see if (prefix + L + suffix) forms a valid word
		const rl_node& prefix_node = dawg.nodearray.items[prefix_node_index];
		for (int32 edge_index = 0; edge_index < prefix_node.next_by_letter.size; edge_index++)
		{
			const rl_edgemap_item& edge = prefix_node.next_by_letter.items[edge_index];
			const int32 ordinal = edge.letter - 'a';
			assert(ordinal >= 0 && ordinal < 26);
			if (_rl_board_check_suffix(dawg, edge.node_index, board, anchor_index, offset, suffix_len))
			{
				value |= (1 << ordinal);
			}
		}

		return value;
	}
	return RL_CHECKBITS_ANY;
}

static void _rl_board_recompute_checkbits(const rl_dawg& dawg, rl_board& board, int32 anchor_index)
{
	assert(board.letters[anchor_index] == RL_ANCHOR);

	board.checkbits_x[anchor_index] = _rl_board_resolve_checkbits(dawg, board, anchor_index, 1, RL_BLOCKFLAG_PREV_ACROSS, RL_BLOCKFLAG_NEXT_ACROSS);
	board.checkbits_y[anchor_index] = _rl_board_resolve_checkbits(dawg, board, anchor_index, board.size_x, RL_BLOCKFLAG_PREV_DOWN, RL_BLOCKFLAG_NEXT_DOWN);
}

void rl_board_init(rl_board& board, int32 playable_size_x, int32 playable_size_y)
{
	board.size_x = MAX(1, playable_size_x);
	board.size_y = MAX(1, playable_size_y);

	const int32 num_squares = board.size_x * board.size_y;
	board.letters = reinterpret_cast<uint8*>(malloc(num_squares));
	board.blockflags = reinterpret_cast<uint8*>(malloc(num_squares));
	board.checkbits_x = reinterpret_cast<uint32*>(malloc(num_squares * sizeof(uint32)));
	board.checkbits_y = reinterpret_cast<uint32*>(malloc(num_squares * sizeof(uint32)));

	assert(board.letters);	
	assert(board.blockflags);
	assert(board.checkbits_x);
	assert(board.checkbits_y);

	_rl_board_clear(board);
}

void rl_board_free(rl_board& board)
{
	free(board.letters);
	free(board.blockflags);
	free(board.checkbits_x);
	free(board.checkbits_y);
}

int32 rl_board_index(const rl_board& board, int32 playable_x, int32 playable_y)
{
	assert(playable_x >= 0 && playable_x < board.size_x);
	assert(playable_y >= 0 && playable_y < board.size_y);
	return playable_y * board.size_x + playable_x;
}

void rl_board_coord(const rl_board& board, int32 index, int32& out_playable_x, int32& out_playable_y)
{
	assert(index >= 0 && index < board.size_x * board.size_y);

	out_playable_y = index / board.size_x;
	out_playable_x = index - (out_playable_y * board.size_x);
}

int32 rl_board_offset(const rl_board& board, bool across)
{
	return across ? 1 : board.size_x;
}

void rl_board_write(const rl_dawg& dawg, rl_board& board, int32 start_index, bool across, const uint8* s, int32 s_len)
{
	// Get our increment for progressing forward in the direction of the move as well as perpendicular to it
	const int32 offset = rl_board_offset(board, across);
	const int32 cross_offset = rl_board_offset(board, !across);

	const uint8 blockflag_next = across ? RL_BLOCKFLAG_NEXT_ACROSS : RL_BLOCKFLAG_NEXT_DOWN;
	const uint8 blockflag_prev = across ? RL_BLOCKFLAG_PREV_ACROSS : RL_BLOCKFLAG_PREV_DOWN;
	const uint8 cross_blockflag_next = across ? RL_BLOCKFLAG_NEXT_DOWN : RL_BLOCKFLAG_NEXT_ACROSS;
	const uint8 cross_blockflag_prev = across ? RL_BLOCKFLAG_PREV_DOWN : RL_BLOCKFLAG_PREV_ACROSS;

	// Keep track of the squares we've noted as 'dirty' anchors, i.e. blank spaces that need their cross-check bits recomputed
	int32 dirty_anchors[RL_MAX_WORD_LEN * 2 + 2];
	int32 num_dirty_anchors = 0;

	// Iterate forward from the start index, writing the word into the board letter-by-letter
	int32 index = start_index;
	for (int32 letter_index = 0; letter_index < s_len; letter_index++)
	{
		// None of the letters we're playing (except the last) should span two adjacent cells that are blocked: if so, we've been given an illegal move
		assert(letter_index == s_len - 1 || (board.blockflags[index] & blockflag_next) == 0);

		// Only place a letter if the space isn't already occupied
		const uint8 existing_letter = board.letters[index];
		if (existing_letter == RL_BLANK || existing_letter == RL_ANCHOR)
		{
			// Copy the letter into the board's letters array
			board.letters[index] = s[letter_index];

			// Search up and down for crosswords, to find the nearest adjacent blank spaces to the space we just updated; these are now anchors and they need their cross-check bits updated
			num_dirty_anchors = _rl_board_flag_dirty_anchor(board, index, -cross_offset, cross_blockflag_prev, dirty_anchors, num_dirty_anchors, COUNT_OF(dirty_anchors));
			num_dirty_anchors = _rl_board_flag_dirty_anchor(board, index, cross_offset, cross_blockflag_next, dirty_anchors, num_dirty_anchors, COUNT_OF(dirty_anchors));
		}
		else
		{
			// If the tile wasn't blank, it should have the expected letter from the word we're playing; otherwise we've been fed an illegal move
			assert(existing_letter == s[letter_index]);
		}

		// Increment the index to point the next square
		index += offset;
	}

	// Now that the word is fully placed, also find the nearest empty spaces before and after the word, so we can flag anchors and update the perpendicular cross-check bits
	num_dirty_anchors = _rl_board_flag_dirty_anchor(board, start_index, -offset, blockflag_prev, dirty_anchors, num_dirty_anchors, COUNT_OF(dirty_anchors));
	num_dirty_anchors = _rl_board_flag_dirty_anchor(board, index - offset, offset, blockflag_next, dirty_anchors, num_dirty_anchors, COUNT_OF(dirty_anchors));

	// Loop over all the newly-flagged anchor squares and recompute their cross-check bits
	for (int32 array_index = 0; array_index < num_dirty_anchors; array_index++)
	{
		const int32 dirty_anchor_index = dirty_anchors[array_index];
		_rl_board_recompute_checkbits(dawg, board, dirty_anchor_index);
	}
}

void rl_board_block_next(rl_board& board, int32 index, bool across)
{
	const uint8 blockflag_next = across ? RL_BLOCKFLAG_NEXT_ACROSS : RL_BLOCKFLAG_NEXT_DOWN;
	const uint8 blockflag_prev = across ? RL_BLOCKFLAG_PREV_ACROSS : RL_BLOCKFLAG_PREV_DOWN;

	const int32 next_index = index + rl_board_offset(board, across);

	board.blockflags[index] |= blockflag_next;
	board.blockflags[next_index] |= blockflag_prev;
}
