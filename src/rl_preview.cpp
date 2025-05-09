#include "rl_preview.h"

#include <cstring>
#include <cassert>

#include "rl_util.h"
#include "rl_board.h"
#include "rl_rack.h"
#include "rl_search.h"
#include "rl_move.h"

static void _rl_preview_clear_results(rl_preview& preview)
{
	for (int32 i = 0; i < COUNT_OF(preview.results); i++)
	{
		preview.results[i].count = 0;
		preview.results[i].word_len = 0;
	}
}

void rl_preview_init(rl_preview& preview)
{
	memset(&preview, 0, sizeof(preview));
}

void rl_preview_search(rl_preview& preview, const rl_dawg& dawg, const rl_board& board, const rl_rack& rack)
{
	// Zero out any existing search results
	_rl_preview_clear_results(preview);

	// Initialize a buffer for our pattern, so we can constrain the first or last letter
	uint8 pattern[RL_MAX_WORD_LEN];
	memset(pattern, 0, sizeof(pattern));
	pattern[0] = preview.letter;

	// Compute a signed offset that will carry us to the next adjacet space, whether we're expanding forward or in reverse
	const int32 offset = rl_board_offset(board, preview.across) * (preview.forward ? 1 : -1);

	// Figure out the number of contiguous spaces in our desired direction
	int32 max_additional_letters = 0;
	{
		// Check the appropriate blockflag depending on which direction we're iterating
		const uint8 blockflag = preview.across ?
			(preview.forward ? RL_BLOCKFLAG_NEXT_ACROSS : RL_BLOCKFLAG_PREV_ACROSS) :
			(preview.forward ? RL_BLOCKFLAG_NEXT_DOWN : RL_BLOCKFLAG_PREV_DOWN);

		// Count up to find out how long our segment could possibly be, until we're blocked or we hit max length
		for (int32 index = preview.index; (board.blockflags[index] & blockflag) == 0; index += offset)
		{
			max_additional_letters++;
			if (max_additional_letters == RL_MAX_WORD_LEN - 1)
			{
				break;
			}
		}
	}

	// For each valid possible length, search the corresponding section of the board and cache the results
	for (int32 num_additional_letters = preview.min_additional_letters; num_additional_letters <= max_additional_letters; num_additional_letters++)
	{
		// The presence of the first letter (at the starting position of our preview) is implied: we can never make a
		// zero-length play. So our loop counter is zero-indexed, representing the number of *additional* letters, and
		// we use that index in our results array. The segment length (which we pass to rl_search_segment), is one more
		// than that, since the segment includes our original letter.
		const int32 segment_length = 1 + num_additional_letters;

		// If we're moving in reverse, we need to update our pattern for each length so the selected letter is always at the end
		if (!preview.forward)
		{
			memset(pattern, 0, sizeof(pattern));
			pattern[num_additional_letters] = preview.letter;
		}

		// rl_search_segment doesn't support reverse searches; instead we flip our coordinates around as needed so that
		// segment_start is the leftmost/topmost cell in our candidate segment.
		const int32 segment_start = preview.forward ? preview.index : preview.index + (num_additional_letters * offset);

		// Check to see whether the cell is blank: if there's already a letter on the board, it will have been
		// considered as part of the prefix in the previous search that we ran, so we can skip it: running another
		// search would just give us the same set of results.
		const uint8 existing_letter = board.letters[preview.index + (offset * num_additional_letters)];
		if (existing_letter == RL_BLANK || existing_letter == RL_ANCHOR)
		{
			// Run an individual segment search for each candidate segment, and cache the results by length.
			// NOTE: In all likelihood, we could make this code more efficient by building the concept of variable,
			// constrained suffix length into rl_search.cpp. Implementing it at a separate level of abstraction just
			// makes it simpler to write and reason about: rl_search is pretty complex as it is.
			rl_move move;
			const int32 num_legal_moves = rl_search_segment(dawg, board, rack, segment_start, pattern, segment_length, preview.across, move);
			if (num_legal_moves > 0)
			{
				rl_preview_result& result = preview.results[num_additional_letters];
				assert(COUNT_OF(result.word) == RL_MAX_WORD_LEN);
				assert(COUNT_OF(move.word) == RL_MAX_WORD_LEN);

				result.count = num_legal_moves;
				result.segment_start_char_index = (segment_start - move.index) / move.offset;;
				result.word_len = move.word_len;
				memcpy(result.word, move.word, RL_MAX_WORD_LEN);				
			}
		}
	}
}
