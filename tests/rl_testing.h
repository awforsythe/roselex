#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <cstdio>

#include "testing.h"
#include "rl_dawg.h"
#include "rl_rack.h"
#include "rl_distribution.h"

int32 rl_test_dawg_build(rl_dawg& dawg, const char* wordlist_file_contents)
{
#ifdef _WIN32
	// Get the path to a suitable directory for storing temp files (from %TEMP%) etc.
	TCHAR tempdir_path[MAX_PATH];
	if (!GetTempPath(MAX_PATH, tempdir_path))
	{
		return -1;
	}

	// Get a throwaway path for a new a file within that directory
	TCHAR wordlist_path[MAX_PATH];
	if (!GetTempFileName(tempdir_path, TEXT("rltest"), 0, wordlist_path))
	{
		return -1;
	}

	// Open the file and write our desired wordlist into it
	FILE* fp = fopen(wordlist_path, "w");
	if (!fp)
	{
		return -1;
	}
	fputs(wordlist_file_contents, fp);
	fclose(fp);
#else
	// Open a temporary file (mkstemp will mutate wordlist_path) write the wordlist
	char wordlist_path[] = "/tmp/rl_test_wordlist.XXXXXX";
	int fd = mkstemp(wordlist_path);
	if (fd == -1)
	{
		return -1;
	}
	if (write(fd, wordlist_file_contents, strlen(wordlist_file_contents)) == -1)
	{
		close(fd);
		return -1;
	}
	close(fd);
#endif

	// Build a DAWG from that file, assuming the rl_dawg is already initialized
	return rl_dawg_build(dawg, wordlist_path);
}

void rl_test_rack_init(rl_rack& rack, const char* letters)
{
	rl_rack_init(rack);
	for (size_t i = 0, n = strlen(letters); i < n; i++)
	{
		const uint8 letter = letters[i];
		if (letter >= 'a' && letter <= 'z')
		{
			rl_rack_push(rack, letter);
		}
	}
}

void rl_test_distribution_init(rl_distribution& distribution, const char* letters)
{
	uint32 counts[26] = { 0 };
	uint32 sum = 0;
	for (size_t i = 0, n = strlen(letters); i < n; i++)
	{
		const uint8 letter = letters[i];
		if (letter >= 'a' && letter <= 'z')
		{
			counts[letter - 'a']++;
			sum++;
		}
	}
	rl_distribution_init(distribution, counts, sum);
}
