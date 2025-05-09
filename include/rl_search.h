#pragma once

#include "rl_types.h"

struct rl_dawg;
struct rl_rack;
struct rl_board;
struct rl_move;

int32 rl_search_board(const rl_dawg& dawg, const rl_board& board, const rl_rack& rack, rl_move& move);
int32 rl_search_segment(const rl_dawg& dawg, const rl_board& board, const rl_rack& rack, int32 start_index, const uint8* pattern, int32 length, bool across, rl_move& move);
