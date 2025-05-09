#include "rl_move.h"

#include <cstring>

void rl_move_init(rl_move& move)
{
	memset(move.word, 0, sizeof(move.word));
	move.word_len = 0;

	rl_rack_init(move.letters_used);
}
