#include <cstdio>
#include <cstring>

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

	rl_dawg dawg;
	memset(&dawg, 0, sizeof(dawg));
	const int32 num_words = rl_dawg_build(dawg, wordlist_path);
	printf("Loaded %d word(s).\n", num_words);
	return 0;
}
