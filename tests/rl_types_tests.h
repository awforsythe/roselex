#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "testing.h"
#include "rl_types.h"

const char* test_types_sizes()
{
	// Verify that we have the expected size for each type
	t_assert(sizeof(uint8) == 1);
	t_assert(sizeof(int32) == 4);
	t_assert(sizeof(uint32) == 4);
	t_assert(sizeof(uint64) == 8);

	// We use uint8 to represent letters, as lowercase ASCII chars 'a' through 'z'
	const char* word = "abacus";
	const uint8 letter = word[0];
	t_assert(letter == 'a');

	return nullptr;
}

const char* test_types_constants()
{
	// RL_MAX_WORD_LEN should be a sane limit
	t_assert(strlen("helicopter") <= RL_MAX_WORD_LEN);
	t_assert(strlen("a-string-thats-entirely-too-long-to-be-a-valid-word-and-that-should-therefore-not-be-accepted") > RL_MAX_WORD_LEN);
	return nullptr;
}
