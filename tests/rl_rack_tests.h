#pragma once

#include <cstdio>
#include <cstdlib>

#include "testing.h"
#include "rl_rack.h"

#include "rl_types.h"
#include "rl_util.h"

const char* test_rack_init()
{
	rl_rack rack;
	rl_rack_init(rack);
	for (size_t i = 0; i < COUNT_OF(rack.counts); i++)
	{
		t_assert(rack.counts[i] == 0);
	}
	t_assert(rack.sum == 0);
	return nullptr;
}

const char* test_rack_push()
{
	rl_rack rack;
	rl_rack_init(rack);

	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'b');

	t_assert(rack.sum == 3);
	t_assert(rack.counts[0] == 2);
	t_assert(rack.counts[1] == 1);
	t_assert(rack.counts[2] == 0);

	return nullptr;
}

const char* test_rack_find()
{
	rl_rack rack;
	rl_rack_init(rack);
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'b');

	t_assert(rl_rack_find(rack, 'a'));
	t_assert(rl_rack_find(rack, 'b'));
	t_assert(!rl_rack_find(rack, 'c'));

	return nullptr;
}

const char* test_rack_pop()
{
	rl_rack rack;
	rl_test_rack_init(rack, "aab");
	t_assert(rack.sum == 3);

	rl_rack_pop(rack, 'a');
	t_assert(rl_rack_find(rack, 'a'));
	t_assert(rl_rack_find(rack, 'b'));
	t_assert(rack.sum == 2);

	rl_rack_pop(rack, 'a');
	t_assert(!rl_rack_find(rack, 'a'));
	t_assert(rl_rack_find(rack, 'b'));
	t_assert(rack.sum == 1);

	rl_rack_pop(rack, 'b');
	t_assert(!rl_rack_find(rack, 'a'));
	t_assert(!rl_rack_find(rack, 'b'));
	t_assert(rack.sum == 0);
	
	return nullptr;
}

const char* test_rack_subtract()
{
	// Initialize two racks, with B being a subset of A
	rl_rack rack_a;
	rl_test_rack_init(rack_a, "aaabbbcccdefg");
	t_assert(rack_a.sum == 13);

	rl_rack rack_b;
	rl_test_rack_init(rack_b, "abcdefg");
	t_assert(rack_b.sum == 7);

	// Subtracting 'abcdefg' from 'aaabbbcccdefg' should reduce rack_a without
	// affecting rack_b
	rl_rack_subtract(rack_a, rack_b);
	t_assert(rack_a.sum == 6);
	t_assert(rack_b.sum == 7);

	// We should be left with 2 each of a, b, and c
	t_assert(rack_a.counts['a' - 'a'] == 2);
	t_assert(rack_a.counts['b' - 'a'] == 2);
	t_assert(rack_a.counts['c' - 'a'] == 2);

	// Subtract another rack, this time with more letters than are found in the
	// original rack: subtraction should stop and be clamped at 0
	rl_rack rack_c;
	rl_test_rack_init(rack_c, "aaaaaaabzzz");
	rl_rack_subtract(rack_a, rack_c);
	t_assert(rack_a.sum == 3);
	t_assert(rack_a.counts['a' - 'a'] == 0);
	t_assert(rack_a.counts['b' - 'a'] == 1);
	t_assert(rack_a.counts['c' - 'a'] == 2);

	return nullptr;
}
