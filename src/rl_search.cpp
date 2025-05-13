#include "rl_search.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "rl_util.h"
#include "rl_dawg.h"
#include "rl_node.h"
#include "rl_edgemap.h"
#include "rl_rack.h"
#include "rl_board.h"
#include "rl_move.h"

// Quick-and-dirty randomness for testing; temporary
#define WITH_FAVORITE_LETTERS

struct rl_search_ctx
{
	const rl_dawg* dawg;
	const rl_board* board;
	rl_rack rack;
	uint8 pattern[RL_MAX_WORD_LEN];
	uint8 s[RL_MAX_WORD_LEN];
	int32 offset;
	uint8 blockflag_next;
	uint8 blockflag_prev;
	uint32* checkbits_array;
	int32 anchor_index;
	int32 required_prefix_len;
	int32 required_suffix_len;
#ifdef WITH_FAVORITE_LETTERS
	uint8 favorite_letters[4];
	bool use_favorite_letters;
	int32 prev_favorite_score;
#endif

	int32 num_legal_moves;
	rl_move* move;
};

static void _rl_accept_move(rl_search_ctx& ctx, int32 s_len, int32 start_index)
{
	ctx.num_legal_moves++;

	rl_move& move = *ctx.move;

#ifdef WITH_FAVORITE_LETTERS
	bool should_adopt = s_len > move.word_len;
	if (ctx.use_favorite_letters)
	{
		int32 favorite_score = 0;
		for (int32 letter_index = 0; letter_index < s_len; letter_index++)
		{
			for (int32 favorite_index = 0; favorite_index < COUNT_OF(ctx.favorite_letters); favorite_index++)
			{
				if (ctx.s[letter_index] == ctx.favorite_letters[favorite_index])
				{
					favorite_score++;
				}
			}
		}

		if (favorite_score > ctx.prev_favorite_score)
		{
			ctx.prev_favorite_score = favorite_score;
			should_adopt = true;
		}
	}

	if (should_adopt)
#endif
	{
		move.index = start_index;
		move.offset = ctx.offset;
		memcpy(move.word, ctx.s, s_len);
		move.word_len = s_len;

		rl_rack_init(move.letters_used);
		int32 square_index = start_index;
		for (int32 letter_index = 0; letter_index < move.word_len; letter_index++)
		{
			if (move.word[letter_index] != ctx.board->letters[square_index])
			{
				rl_rack_push(move.letters_used, move.word[letter_index]);
			}
			square_index += ctx.offset;
		}
	}
}

static void _rl_consider_word(rl_search_ctx& ctx, int32 s_len, int32 square_index, int32 suffix_len)
{
	if (ctx.required_suffix_len < 0 || suffix_len == ctx.required_suffix_len)
	{
		const int32 start_index = square_index - s_len * ctx.offset;
		_rl_accept_move(ctx, s_len, start_index);
	}
}

static void _rl_build_suffix(rl_search_ctx& ctx, int32 s_len, int32 node_index, int32 square_index)
{
	assert(s_len < COUNT_OF(ctx.s));

	// We know that we have a valid prefix (even if zero-length), and from that prefix, we're trying to build a suffix
	// which gives us a word that meets the criteria for our current search. The prefix is represented by node_index:
	// this is the DAWG node that's positioned along the sequence of edges that spells out our prefix.
	const rl_node& node = ctx.dawg->nodearray.items[node_index];

	// square_index points to the square on the board that our suffix will start at. We know that this cell has a
	// valid prefix and is within the bounds of the board. If that cell already has a letter in it, we're trying to
	// figure out whether that letter is present as an edge leading out of the current DAWG node. If the cell is
	// blank, we're checking the edges against our rack to see which letters we could play in this cell.
	assert(square_index >= 0 && square_index < ctx.board->size_x * ctx.board->size_y);

	// It's the edges *between* nodes, not the nodes themselves, that correspond to letters in a word. So the current
	// node represents our prefix, the edges leading out from that node represent the possible letters we could play in
	// the current square, and the destination nodes at the end of each of those edges represent the new prefixes that
	// we would arrive at in subsequent recursive calls, *if* we continue traversing the DAWG from this point.
	bool can_continue = true;

	// One reason for us to continue no further down this branch of the DAWG would be if our current cell is blocked
	// from reaching the next square across. We can check the blockflags for our current cell to see whether a word
	// can be played contiguously across this cell and the next.
	if (ctx.board->blockflags[square_index] & ctx.blockflag_next)
	{
		can_continue = false;
	}

	// Another reason would be if we're trying to find a suffix of a specific length: if we're currently at that
	// length, we don't want to continue any further.
	const int32 suffix_len = (square_index - ctx.anchor_index) / ctx.offset;
	if (ctx.required_suffix_len >= 0 && suffix_len > ctx.required_suffix_len)
	{
		can_continue = false;
	}

	// Now we know whether we're going to make any more recursive calls from this point, to take us further down this
	// branch of the DAWG. But either way, we still need to consider *this* square on the board, and *this* set of
	// edges leading out from the DAWG node that represents our current prefix. So we'll check to see what's in that
	// square currently: it either has a letter in it, or it doesn't.
	const uint8 existing_letter = ctx.board->letters[square_index];
	if (existing_letter != RL_BLANK && existing_letter != RL_ANCHOR)
	{
		// If the square has a letter in it already, then our suffix *must* start with that letter: see if there's an
		// edge leading out of our current DAWG node that's labeled with that letter
		assert(existing_letter >= 'a' && existing_letter <= 'z');
		const int32 next_node_index = rl_edgemap_find(node.next_by_letter, existing_letter);
		if (next_node_index >= 0)
		{
			// If there's a valid edge for that letter, write it into our temporary buffer at the current offset
			ctx.s[s_len] = existing_letter;

			// If the node it leads to is terminal, (prefix + suffix) gives us a valid word: check to see if we want
			// to accept it as a valid move for this search
			if (ctx.dawg->nodearray.items[next_node_index].is_word)
			{
				_rl_consider_word(ctx, s_len + 1, square_index + ctx.offset, suffix_len + 1);
			}

			// Continue traversing the DAWG, by making another recursive call, only if we're clear to do so
			if (can_continue)
			{
				_rl_build_suffix(ctx, s_len + 1, next_node_index, square_index + ctx.offset);
			}
		}
	}
	else
	{
		// If the square doesn't have a letter in it, we can play any letter from our rack, so long as it's permitted
		// by the relevant set of cross-check bits (meaning that any cross-words it forms are valid)
		const uint32 checkbits = ctx.checkbits_array[square_index];
		for (int32 edge_index = 0; edge_index < node.next_by_letter.size; edge_index++)
		{
			// Check each outgoing edge from the current node: this tells us what letters we can add to our current
			// prefix while still having a chance of ending up with a valid word. If we've been supplied with a pattern
			// that we must match, enforce that constraint here as well.
			const rl_edgemap_item& edge = node.next_by_letter.items[edge_index];
			if (ctx.pattern[s_len] < 'a' || ctx.pattern[s_len] == edge.letter)
			{
				// If the letter satisfies our cross-check bits and we have that letter in our rack, temporarily remove
				// the letter from the rack and push a new stack frame where our prefix is extended by that letter, and
				// we're trying to find a new suffix (one character smaller) for *that* prefix.
				const uint32 letter_bit = 1 << (edge.letter - 'a');
				if ((letter_bit & checkbits) && rl_rack_pop(ctx.rack, edge.letter))
				{
					// Write the letter we're currently testing into our temporary buffer at the current offset
					ctx.s[s_len] = edge.letter;

					// If the node it leads to is terminal, (prefix + suffix) gives us a valid word: check to see if we want
					// to accept it as a valid move for this search
					if (ctx.dawg->nodearray.items[edge.node_index].is_word)
					{
						_rl_consider_word(ctx, s_len + 1, square_index + ctx.offset, suffix_len + 1);
					}

					if (can_continue)
					{
						_rl_build_suffix(ctx, s_len + 1, edge.node_index, square_index + ctx.offset);
					}

					// Make sure the letter gets added back to the rack at the end of the stack frame
					rl_rack_push(ctx.rack, edge.letter);
				}
			}
		}
	}
}

static void _rl_build_prefix(rl_search_ctx& ctx, int32 s_len, int32 node_index, int32 limit)
{
	if (ctx.required_prefix_len < 0 || s_len == ctx.required_prefix_len)
	{
		_rl_build_suffix(ctx, s_len, node_index, ctx.anchor_index);
	}

	if (limit > 0)
	{
		const rl_node& node = ctx.dawg->nodearray.items[node_index];
		for (int32 edge_index = 0; edge_index < node.next_by_letter.size; edge_index++)
		{
			const rl_edgemap_item& edge = node.next_by_letter.items[edge_index];
			if (ctx.pattern[s_len] < 'a' || ctx.pattern[s_len] == edge.letter)
			{
				if (rl_rack_pop(ctx.rack, edge.letter))
				{
					ctx.s[s_len] = edge.letter;
					_rl_build_prefix(ctx, s_len + 1, edge.node_index, limit - 1);
					rl_rack_push(ctx.rack, edge.letter);
				}
			}
		}
	}
}

static void _rl_search_anchor(rl_search_ctx& ctx, int32 num_preceding_blanks, int32 num_preceding_letters)
{
	// If our anchor is preceded by one or more letters, those letters form the prefix
	assert(num_preceding_letters <= RL_MAX_WORD_LEN);
	if (num_preceding_letters > 0)
	{
		// Copy that existing prefix into our buffer (letter-by-letter, since down words aren't stored contiguously), and traverse the DAWG to the corresponding node
		int32 s_len = 0;
		int32 square_index = ctx.anchor_index - (num_preceding_letters * ctx.offset);
		int32 node_index = 0;
		while (s_len < num_preceding_letters)
		{
			ctx.s[s_len] = ctx.board->letters[square_index];
			node_index = rl_edgemap_find(ctx.dawg->nodearray.items[node_index].next_by_letter, ctx.s[s_len]);
			if (node_index < 0)
			{
				return;
			}
			s_len++;
			square_index += ctx.offset;
		}

		// Now find all valid suffixes for that prefix
		_rl_build_suffix(ctx, s_len, node_index, ctx.anchor_index);
	}
	else
	{
		// Otherwise, we need to find all possible prefixes from our hand, then search for legal suffixes branching off those
		_rl_build_prefix(ctx, 0, 0, num_preceding_blanks);
	}
}

static void _rl_search_line(rl_search_ctx& ctx, int32 start_index, int32 end_index)
{
	// Iterate across the line, counting the contiguous blanks/letters along the way so we don't have to backtrack
	int32 num_contiguous_blanks = 0;
	int32 num_contiguous_letters = 0;
	for (int32 index = start_index; index < end_index; index += ctx.offset)
	{
		// If moves can't be played contiguously across the previous cell and this cell, make a clean break
		if ((ctx.board->blockflags[index] & ctx.blockflag_prev) != 0)
		{
			num_contiguous_blanks = 0;
			num_contiguous_letters = 0;
		}

		// When we hit an anchor, start searching for valid moves that can be built from that anchor
		const uint8 letter = ctx.board->letters[index];
		if (letter == RL_ANCHOR)
		{
			ctx.anchor_index = index;
			_rl_search_anchor(ctx, num_contiguous_blanks, num_contiguous_letters);
			num_contiguous_blanks = 0;
			num_contiguous_letters = 0;
		}
		else if (letter == RL_BLANK)
		{
			num_contiguous_blanks++;
			num_contiguous_letters = 0;
		}
		else
		{
			assert(letter >= 'a' && letter <= 'z');
			num_contiguous_blanks = 0;
			num_contiguous_letters++;
		}
	}
}

int32 rl_search_board(const rl_dawg& dawg, const rl_board& board, const rl_rack& rack, rl_move& move)
{
	// Establish a context struct to wrap up the data describing our search, and to hold a string buffer and a mutable copy of the rack
	rl_search_ctx ctx;
	ctx.dawg = &dawg;
	ctx.board = &board;
	memcpy(&ctx.rack, &rack, sizeof(rl_rack));
	memset(ctx.pattern, 0, sizeof(ctx.pattern));
	memset(ctx.s, 0, sizeof(ctx.s));
	ctx.offset = 0;
	ctx.blockflag_next = 0;
	ctx.blockflag_prev = 0;
	ctx.checkbits_array = nullptr;
	ctx.anchor_index = -1;
	ctx.required_prefix_len = -1;
	ctx.required_suffix_len = -1;
	ctx.num_legal_moves = 0;
	rl_move_init(move);
	ctx.move = &move;
#ifdef WITH_FAVORITE_LETTERS
	ctx.use_favorite_letters = false;
	ctx.prev_favorite_score = -1;
#endif

	// Start at the top and go down the board to search each row for across moves
	ctx.offset = rl_board_offset(*ctx.board, true);
	ctx.blockflag_next = RL_BLOCKFLAG_NEXT_ACROSS;
	ctx.blockflag_prev = RL_BLOCKFLAG_PREV_ACROSS;
	ctx.checkbits_array = ctx.board->checkbits_y;
	for (int32 playable_y = 0; playable_y < ctx.board->size_y; playable_y++)
	{
		const int32 start_index = rl_board_index(*ctx.board, 0, playable_y);
		const int32 upper_bound = start_index + ctx.board->size_x * ctx.offset;
		_rl_search_line(ctx, start_index, upper_bound);
	}

	// Start at the left edge and go across the board to search each column for down moves
	ctx.offset = rl_board_offset(*ctx.board, false);
	ctx.blockflag_next = RL_BLOCKFLAG_NEXT_DOWN;
	ctx.blockflag_prev = RL_BLOCKFLAG_PREV_DOWN;
	ctx.checkbits_array = ctx.board->checkbits_x;
	for (int32 playable_x = 0; playable_x < ctx.board->size_x; playable_x++)
	{
		const int32 start_index = rl_board_index(*ctx.board, playable_x, 0);
		const int32 upper_bound = start_index + ctx.board->size_y * ctx.offset;
		_rl_search_line(ctx, start_index, upper_bound);
	}

	return ctx.num_legal_moves;
}

int32 rl_search_segment(const rl_dawg& dawg, const rl_board& board, const rl_rack& rack, int32 start_index, const uint8* pattern, int32 length, bool across, rl_move& move)
{
	// Get the relevant details for the segment of the row/column we're searching
	const int32 offset = rl_board_offset(board, across);
	const uint8 blockflag_next = across ? RL_BLOCKFLAG_NEXT_ACROSS : RL_BLOCKFLAG_NEXT_DOWN;

	// Scan across the segment and see what's currently occupying those squares
	int32 segment_num_blanks = 0;
	int32 segment_num_anchors = 0;
	int32 segment_num_letters = 0;
	int32 segment_first_anchor_index = -1;
	for (int32 segment_offset = 0; segment_offset < length; segment_offset++)
	{
		// Accumulate some stats about the selected segment, and note the first ancchor we see
		const int32 index = start_index + (offset * segment_offset);
		const uint8 letter = board.letters[index];
		if (letter == RL_BLANK)
		{
			segment_num_blanks++;
		}
		else if (letter == RL_ANCHOR)
		{
			segment_num_anchors++;
			if (segment_first_anchor_index == -1)
			{
				segment_first_anchor_index = index;
			}
		}
		else
		{
			// Anything else on the board should be a valid letter
			assert(letter >= 'a' && letter <= 'z');
			segment_num_letters++;
		}

		// If we're blocked from reaching the next adjacent cell at any point, our segment has to be shortened to end there
		if ((board.blockflags[index] & blockflag_next) != 0)
		{
			assert(segment_offset + 1 <= length);
			length = segment_offset + 1;
			break;
		}
	}

	// If we've selected a segment with no blanks and no anchors (i.e. all letters), we can't place anything there
	if (segment_num_letters == length)
	{
		return 0;
	}

	// Initialize our search context
	rl_search_ctx ctx;
	ctx.dawg = &dawg;
	ctx.board = &board;
	memcpy(&ctx.rack, &rack, sizeof(rl_rack));
	memset(ctx.pattern, 0, sizeof(ctx.pattern));
	ctx.offset = offset;
	ctx.blockflag_next = blockflag_next;
	ctx.blockflag_prev = across ? RL_BLOCKFLAG_PREV_ACROSS : RL_BLOCKFLAG_PREV_DOWN;
	ctx.checkbits_array = across ? board.checkbits_y : board.checkbits_x;
	ctx.anchor_index = -1;
	ctx.required_prefix_len = -1;
	ctx.required_suffix_len = -1;
	ctx.num_legal_moves = 0;
	rl_move_init(move);
	ctx.move = &move;
#ifdef WITH_FAVORITE_LETTERS
	for (int32 i = 0; i < COUNT_OF(ctx.favorite_letters); i++)
	{
		ctx.favorite_letters[i] = 'a' + rand() % 26;
	}
	ctx.use_favorite_letters = true;
	ctx.prev_favorite_score = -1;
#endif

	// If at least one square does not contain a letter, then one of two things is true:
	// - Either we have one or more anchors within the segment...
	// - ...or the segment has no letters at all, i.e. it's entirely blank.
	assert(segment_num_anchors > 0 || segment_num_letters == 0);
	assert(segment_num_anchors == 0 || segment_first_anchor_index >= 0);

	// If there are anchors, build a word from the first anchor, constraining the prefix/suffix length as needed to fill the selected segment
	if (segment_first_anchor_index >= 0)
	{
		// If there are any letters behind the anchor, they should form our word's prefix; otherwise we want a prefix whose length is exactly the number of blanks from the start of the segment to the anchor
		int32 num_preceding_letters = 0;
		{
			int32 index = segment_first_anchor_index;
			while (true)
			{
				// If we're blocked from iterating backward, we have no more cells to consider for our prefix
				if ((board.blockflags[index] & ctx.blockflag_prev) != 0)
				{
					break;
				}

				// If we've hit a blank letter, there are no more letters in the prefix
				const int32 prev_index = index - offset;
				const uint8 prev_letter = board.letters[prev_index];
				if (prev_letter == RL_BLANK || prev_letter == RL_ANCHOR)
				{
					break;
				}

				// Otherwise, this preceding cell should contain a letter which forms part of the prefix for our move at this anchor
				assert(prev_letter >= 'a' && prev_letter <= 'z');
				num_preceding_letters++;
				index = prev_index;
			}
		}

		// If the final square in the segment is blank (and not an anchor, meaning it's not adjacent to a letter), then our suffix length should be exactly the distance from (and including) the anchor to that square
		const int32 end_index = start_index + offset * length;
		int32 required_suffix_length = (end_index - segment_first_anchor_index) / offset;

		// However, if the segment ends with an anchor or a letter, we need to count all contiguous letters that follow as part of the word we're playing, increasing our required suffix length
		if (board.letters[end_index - offset] != RL_BLANK && (board.blockflags[end_index - offset] & ctx.blockflag_next) == 0)
		{
			for (int32 index = end_index; true; index += offset)
			{
				// If the next letter is blank, there are no more letters that need to be included in the suffix
				const uint8 next_letter = board.letters[index];
				if (next_letter == RL_BLANK || next_letter == RL_ANCHOR)
				{
					break;
				}

				// Otherwise, this cell should contain a letter which forms part of the suffix for our move at this anchor
				assert(next_letter >= 'a' && next_letter <= 'z');
				required_suffix_length++;

				// If we're blocked from iterating forward, we have no more cells to consider for our suffix
				if ((board.blockflags[index] & ctx.blockflag_next) != 0)
				{
					break;
				}
			}
		}

		// Start our move search algorithm, for just that single anchor
		ctx.anchor_index = segment_first_anchor_index;
		ctx.required_prefix_len = num_preceding_letters > 0 ? 0 : (segment_first_anchor_index - start_index) / ctx.offset;
		ctx.required_suffix_len = required_suffix_length;
		if (pattern)
		{
			memcpy(ctx.pattern + num_preceding_letters, pattern, length);
		}
		_rl_search_anchor(ctx, ctx.required_prefix_len, num_preceding_letters);
		return ctx.num_legal_moves;
	}

	// Otherwise, the square is entirely blank with no anchors
	assert(segment_num_anchors == 0 && segment_num_letters == 0);
	assert(segment_num_blanks == length);

	// This means we can treat the start square as anchor and build a word with a 0-length prefix and a suffix the length of our segment
	ctx.anchor_index = start_index;
	ctx.required_suffix_len = length;
	if (pattern)
	{
		memcpy(ctx.pattern, pattern, length);
	}
	_rl_search_anchor(ctx, 0, 0);
	return ctx.num_legal_moves;
}
