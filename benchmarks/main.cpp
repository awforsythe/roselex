#include <cstdio>
#include <cstring>

#include <chrono>

#include "rl_types.h"
#include "rl_dawg.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "ERROR: Path to word list must be specified as first argument.\n");
		return 1;
	}
	const char* wordlist_path = argv[1];

	const auto start = std::chrono::high_resolution_clock::now();

	rl_dawg dawg;
	memset(&dawg, 0, sizeof(dawg));
	const int32 num_words = rl_dawg_build(dawg, wordlist_path);
	printf("Loaded %d word(s).\n", num_words);

	const auto elapsed = std::chrono::high_resolution_clock::now() - start;
	const std::chrono::nanoseconds elapsed_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
	printf("Elapsed: %lld ns\n", static_cast<long long int>(elapsed_nanos.count()));

	rl_dawg_free(dawg);

	return 0;
}
