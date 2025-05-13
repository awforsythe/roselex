#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <chrono>

#include "rl_types.h"
#include "rl_dawg.h"
#include "rl_bag.h"
#include "rl_rack.h"
#include "rl_board.h"
#include "rl_move.h"
#include "rl_search.h"

struct TimeSample {
	std::chrono::high_resolution_clock::time_point start_;

	void start() {
		start_ = std::chrono::high_resolution_clock::now();
	}

	long long int stop() {
		const auto elapsed = std::chrono::high_resolution_clock::now() - start_;
		return static_cast<long long int>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
	}
};

const int32 BOARD_SIZE_X = 500;
const int32 BOARD_SIZE_Y = BOARD_SIZE_X;

const int32 NUM_TILES_TO_DRAW = 50;
const int32 NUM_MOVES_TO_PLAY = 500;

int main(int argc, char* argv[])
{
	TimeSample ts;
	srand(0xfeeefeee);

	if (argc < 2)
	{
		fprintf(stderr, "ERROR: Path to word list must be specified as first argument.\n");
		return 1;
	}
	const char* wordlist_path = argv[1];
	
	bool csv_output = false;
	if (argc > 2 && strcmp(argv[2], "--csv") == 0)
	{
		csv_output = true;	
	}

	ts.start();
	rl_dawg dawg;
	memset(&dawg, 0, sizeof(dawg));
	const int32 num_words = rl_dawg_build(dawg, wordlist_path);
	const long long elapsed_load = ts.stop();

	// Draw the desired number of tiles, using the default letter distribution
	rl_bag bag;
	rl_bag_init(bag);

	rl_rack rack;
	rl_rack_init(rack);
	for (int32 i = 0; i < NUM_TILES_TO_DRAW; i++)
	{
		rl_rack_push(rack, rl_bag_draw(bag));
	}

	// Prepare a board with the desired dimensions
	rl_board board;
	rl_board_init(board, BOARD_SIZE_X, BOARD_SIZE_Y);

	// Hardcode an initial move to populate the board
	rl_move move;
	rl_move_init(move);
	move.index = rl_board_index(board, 4, 4);
	move.offset = 1;
	move.word[0] = 'y';
	move.word[1] = 'e';
	move.word[2] = 's';
	move.word[3] = 't';
	move.word[4] = 'e';
	move.word[5] = 'r';
	move.word[6] = 'd';
	move.word[7] = 'a';
	move.word[8] = 'y';
	move.word_len = 9;
	rl_rack_push(move.letters_used, 'y');
	rl_rack_push(move.letters_used, 'e');
	rl_rack_push(move.letters_used, 's');
	rl_rack_push(move.letters_used, 't');
	rl_rack_push(move.letters_used, 'e');
	rl_rack_push(move.letters_used, 'r');
	rl_rack_push(move.letters_used, 'd');
	rl_rack_push(move.letters_used, 'a');
	rl_rack_push(move.letters_used, 'y');

	ts.start();
	int32 num_moves_played = 0;
	for (int32 i = 0; i < NUM_MOVES_TO_PLAY; i++)
	{
		rl_board_write(dawg, board, move.index, move.offset == 1, move.word, move.word_len);
		rl_rack_subtract(rack, move.letters_used);
		num_moves_played++;
		for (int32 j = 0; j < move.word_len; j++)
		{
			rl_rack_push(rack, rl_bag_draw(bag));
		}

		const int32 num_moves_found = rl_search_board(dawg, board, rack, move);
		if (num_moves_found == 0)
		{
			break;
		}
	}
	const long long elapsed_moves = ts.stop();

	if (csv_output)
	{
		printf("%d,%d,%d,%lld,%lld\n", num_words, NUM_TILES_TO_DRAW, NUM_MOVES_TO_PLAY, elapsed_load, elapsed_moves);
	}
	else
	{
		printf("words: %d\n", num_words);
		printf("tiles: %d\n", NUM_TILES_TO_DRAW);
		printf("moves: %d\n", NUM_MOVES_TO_PLAY);
		printf("elapsed(load): %lld ns\n", elapsed_load);
		printf("elapsed(moves): %lld ns\n", elapsed_moves);
	}

	rl_board_free(board);
	rl_dawg_free(dawg);

	return 0;
}
