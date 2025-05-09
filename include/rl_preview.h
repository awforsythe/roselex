#pragma once

#include "rl_types.h"

struct rl_preview_result
{
	int32 count; //!< Number of results of this length that were found
	int32 segment_start_char_index; //!< Index into the word buffer that corresponds to letter at the start of our segment
	int32 word_len; //!< Length of the word written into the word buffer
	uint8 word[RL_MAX_WORD_LEN]; //!< A single search result for this length
};

struct rl_preview
{
	int32 index; //!< Coordinate on the board where our cursor is positioned
	int32 min_additional_letters; //!< Minimum number of additional letters to make a valid play; i.e. distance to playable territory
	uint8 letter; //!< The active letter we're previewing at that position: either the first or last letter for our segment
	bool across; //!< Whether we intend to play moves across or down from that position
	bool forward; //!< Whether our cursor is the start of the segment we want to search, or the end

	rl_preview_result results[RL_MAX_WORD_LEN]; //!< The results our search yielded, indexed according to the number of addititonal letters used to form the segment (e.g. results[0] is the segment of length 1 consisting of just the active letter itself; results[1] is a segment of length 2; etc.)
};

void rl_preview_init(rl_preview& preview);
void rl_preview_search(rl_preview& preview, const struct rl_dawg& dawg, const struct rl_board& board, const struct rl_rack& rack);
