#pragma once

#include <cstdio>
#include <cstdlib>

#include "testing.h"
#include "rl_bag.h"

#include "rl_types.h"
#include "rl_util.h"

const char* test_bag_init()
{
	rl_bag bag;
	rl_bag_init(bag);

	// Once initialized, a bag should have valid weights controlling the frequency at which each letter is drawn
	int32 total = 0;
	t_assert(COUNT_OF(bag.weights) == 26);
	for (size_t i = 0; i < COUNT_OF(bag.weights); i++)
	{
		t_assert(bag.weights[i] > 0);
		total += bag.weights[i];
	}

	// weight_sum should equal the sum of all individual weights
	t_assert(bag.weight_sum == total);

	return nullptr;
}

const char* test_bag_draw()
{
	rl_bag bag;
	rl_bag_init(bag);

	// rl_bag_draw should give us a random letter (seeds will need to be updated if default weights or the letter-picking algorithm change at all)
	srand(0xeeee);
	const uint8 letter_g = rl_bag_draw(bag);
	t_assert(letter_g == 'g');

	srand(0xffff);
	const uint8 letter_r = rl_bag_draw(bag);
	t_assert(letter_r == 'r');

	return nullptr;
}
