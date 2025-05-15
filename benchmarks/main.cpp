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

int32 seed = 0xfeeefeee;

int32 board_size_x = 500;
int32 board_size_y = board_size_x;

int32 first_move_x = 4;
int32 first_move_y = 4;
const char* first_move_word = "yesterday";

int32 num_tiles_to_draw = 50;
int32 num_moves_to_play = 500;
int32 num_searches_to_play = 10;

bool print_board = false;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "ERROR: Path to word list must be specified as first argument.\n");
		return 1;
	}
	const char* wordlist_path = argv[1];

	for (int32 i = 2; i < argc; i++)
	{
		if (strstr(argv[i], "--seed="))
		{
			seed = atoi(argv[i]+7);
		}
		else if (strstr(argv[i], "--board-size-x="))
		{
			board_size_x = atoi(argv[i]+15);
		}
		else if (strstr(argv[i], "--board-size-y="))
		{
			board_size_y = atoi(argv[i]+15);
		}
		else if (strstr(argv[i], "--first-move-x="))
		{
			first_move_x = atoi(argv[i]+15);
		}
		else if (strstr(argv[i], "--first-move-y="))
		{
			first_move_y = atoi(argv[i]+15);
		}
		else if (strstr(argv[i], "--first-move-word="))
		{
			first_move_word = argv[i]+18;
		}
		else if (strstr(argv[i], "--num-tiles="))
		{
			num_tiles_to_draw = atoi(argv[i]+12);
		}
		else if (strstr(argv[i], "--num-moves="))
		{
			num_moves_to_play = atoi(argv[i]+12);
		}
		else if (strstr(argv[i], "--num-searches="))
		{
			num_searches_to_play = atoi(argv[i]+15);
		}
		else if (strstr(argv[i], "--print-board"))
		{
			print_board = true;
		}
	}
	printf("seed: %d\n", seed);
	printf("board-size-x: %d\n", board_size_x);
	printf("board-size-y: %d\n", board_size_y);
	printf("num-tiles: %d\n", num_tiles_to_draw);
	printf("num-moves: %d\n", num_moves_to_play);
	printf("num-searches: %d\n", num_searches_to_play);
	printf("first-move-x: %d\n", first_move_x);
	printf("first-move-y: %d\n", first_move_y);
	printf("first-move-word: %s\n", first_move_word);

	TimeSample ts;
	srand(seed);

	ts.start();
	rl_dawg dawg;
	memset(&dawg, 0, sizeof(dawg));
	const int32 num_words = rl_dawg_build(dawg, wordlist_path);
	const long long elapsed_load = ts.stop();
	printf("num-words: %d\n", num_words);
	printf("elapsed(load): %lld ns\n", elapsed_load);

	// Draw the desired number of tiles, using the default letter distribution
	rl_bag bag;
	rl_bag_init(bag);

	rl_rack rack;
	rl_rack_init(rack);
	for (int32 i = 0; i < num_tiles_to_draw; i++)
	{
		rl_rack_push(rack, rl_bag_draw(bag));
	}

	// Prepare a board with the desired dimensions
	rl_board board;
	rl_board_init(board, board_size_x, board_size_y);

	// Hardcode an initial move to populate the board
	rl_move move;
	rl_move_init(move);
	move.index = rl_board_index(board, first_move_x, first_move_y);
	move.offset = 1;
	move.word_len = strlen(first_move_word);
	for (int32 i = 0; i < move.word_len; i++)
	{
		move.word[i] = first_move_word[i];
		rl_rack_push(move.letters_used, first_move_word[i]);
	}

	// Play the desired number of moves, zigzagging across the board and searching in
	// arbitrary segments to chain the moves together
	ts.start();
	int32 num_moves_played = 0;
	while (true)
	{
		// Play the current move
		rl_board_write(dawg, board, move.index, move.offset == 1, move.word, move.word_len);
		rl_rack_subtract(rack, move.letters_used);
		num_moves_played++;

		// Replenish the rack to replace the letters played
		for (int32 j = 0; j < move.word_len; j++)
		{
			rl_rack_push(rack, rl_bag_draw(bag));
		}

		if (num_moves_played == num_moves_to_play)
		{
			break;
		}

		// Find the next move to play, by searching perpendicular to the move we just
		// played, near the end of the word
		const bool across = num_moves_played % 2 == 0;
		const int32 advance_from_start_of_prev_move = move.word_len - (rand() % 2) - 1;
		const int32 index_of_intersect_with_prev_move = move.index + (advance_from_start_of_prev_move * move.offset);
		const int32 prefix_len = (rand() % 2) + 1;
		const int32 retreat_to_start_of_next_move = rl_board_offset(board, across) * prefix_len;
		const int32 next_move_start_index = index_of_intersect_with_prev_move - retreat_to_start_of_next_move;
		const int32 next_move_length = (rand() % 3) + 5;
		const int32 num_moves_found = rl_search_segment(dawg, board, rack, next_move_start_index, nullptr, next_move_length, across, move);
		printf("move(%d)found: %d\n", num_moves_played, num_moves_found);
		if (num_moves_found == 0)
		{
			break;
		}
	}
	const long long elapsed_moves = ts.stop();

	printf("num-moves-played: %d\n", num_moves_played);
	printf("elapsed(moves): %lld ns\n", elapsed_moves);

	//
	ts.start();
	int32 num_searches_played = 0;
	for (int32 i = 0; i < num_searches_to_play; i++)
	{
		const int32 num_moves_found = rl_search_board(dawg, board, rack, move);
		printf("search(%d)found: %d\n", i, num_moves_found);
		if (num_moves_found == 0)
		{
			break;
		}

		rl_board_write(dawg, board, move.index, move.offset == 1, move.word, move.word_len);
		rl_rack_subtract(rack, move.letters_used);
		num_searches_played++;
		for (int32 j = 0; j < move.word_len; j++)
		{
			rl_rack_push(rack, rl_bag_draw(bag));
		}
	}
	const long long elapsed_searches = ts.stop();

	printf("num-searches-played: %d\n", num_searches_played);
	printf("elapsed(searches): %lld ns\n", elapsed_searches);

	if (print_board)
	{
		for (int32 y = 0; y < board_size_y; y++)
		{
			for (int32 x = 0; x < board_size_x; x++)
			{
				const int32 index = rl_board_index(board, x, y);
				if (board.letters[index] == RL_BLANK || board.letters[index] == RL_ANCHOR)
				{
					putchar('.');
				}
				else
				{
					putchar(board.letters[index]);
				}
			}
			putchar('\n');
		}
	}

	rl_board_free(board);
	rl_dawg_free(dawg);

	return 0;
}
