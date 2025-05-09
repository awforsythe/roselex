#pragma once

#include "rl_types.h"

static const uint8 RL_BLANK = 176;
static const uint8 RL_ANCHOR = 177;

static const uint32 RL_CHECKBITS_ANY = (1 << 26) - 1;

static const uint8 RL_BLOCKFLAG_NEXT_ACROSS = 0x01;
static const uint8 RL_BLOCKFLAG_NEXT_DOWN   = 0x02;
static const uint8 RL_BLOCKFLAG_PREV_ACROSS = 0x04;
static const uint8 RL_BLOCKFLAG_PREV_DOWN   = 0x08;

struct rl_dawg;

struct rl_board
{
	int32 size_x; // Width of the board including a column of border squares on either side, i.e. playable_size_x + 2 - use rl_board_index to convert from playable x,y coords to a flat index
	int32 size_y; // Height of the board including a row of border squares at top and bottom, i.e. playable_size_y + 2 - use rl board_index to convert from playable x,y coords to a flat index
	uint8* letters; // Character values for letter placed on the board: either RL_BLANK, RL_ANCHOR, or a valid lowercase letter 'a'-'z'
	uint8* blockflags; // Flags for each cell, indicating whether the cell is blocked from reaching its neighbor in each of the four directions
	uint32* checkbits_x; // Allowable letters when playing a word DOWN, such that the tile played in this square forms a valid across move with the adjacent tiles already on the board
	uint32* checkbits_y; // Allowable letters when playing a word ACROSS, such that the tile played in this square forms a valid down move with the adjacent titles already on the board
};

void rl_board_init(rl_board& board, int32 playable_size_x, int32 playable_size_y);
void rl_board_free(rl_board& board);
int32 rl_board_index(const rl_board& board, int32 playable_x, int32 playable_y);
void rl_board_coord(const rl_board& board, int32 index, int32& out_playable_x, int32& out_playable_y);
int32 rl_board_offset(const rl_board& board, bool across);
void rl_board_write(const rl_dawg& dawg, rl_board& board, int32 start_index, bool across, const uint8* s, int32 s_len);
void rl_board_block_next(rl_board& board, int32 index, bool across);
