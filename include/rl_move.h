#pragma once

#include "rl_types.h"
#include "rl_rack.h"

struct rl_move
{
	int32 index;
	int32 offset;
	uint8 word[RL_MAX_WORD_LEN];
	int32 word_len;
	rl_rack letters_used;
};

void rl_move_init(rl_move& move);
