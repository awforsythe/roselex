#pragma once

#include <cstdlib>

#include "testing.h"
#include "rl_testing.h"
#include "rl_distribution.h"

#include "rl_types.h"
#include "rl_rack.h"

const char* test_distribution_init()
{
	rl_distribution distribution;

	uint32 counts[26] = { 0 };
	counts['a' - 'a'] = 44;
	counts['b' - 'a'] = 6;
	counts['c' - 'a'] = 30;
	counts['z' - 'a'] = 20;
	const uint32 sum = 100;

	rl_distribution_init(distribution, counts, sum);
	t_assert(distribution.weights[0] == 0.44f);
	t_assert(distribution.weights[1] == 0.06f);
	t_assert(distribution.weights[2] == 0.3f);
	t_assert(distribution.weights[3] == 0.0f);
	t_assert(distribution.weights[24] == 0.0f);
	t_assert(distribution.weights[25] == 0.2f);

	return nullptr;
}

const char* test_distribution_from_rack()
{
	rl_rack rack;
	rl_rack_init(rack);
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'a');
	rl_rack_push(rack, 'b');
	rl_rack_push(rack, 'b');
	rl_rack_push(rack, 'c');
	rl_rack_push(rack, 'c');
	rl_rack_push(rack, 'd');
	t_assert(rack.sum == 10);

	rl_distribution distribution;
	rl_distribution_from_rack(distribution, rack);
	t_assert(distribution.weights[0] == 0.5f);
	t_assert(distribution.weights[1] == 0.2f);
	t_assert(distribution.weights[2] == 0.2f);
	t_assert(distribution.weights[3] == 0.1f);
	t_assert(distribution.weights[4] == 0.0f);

	return nullptr;
}

const char* test_distribution_compare()
{
	rl_distribution uniform;
	rl_distribution uniform_doubled;
	rl_test_distribution_init(uniform, "abcdefghijklmnopqrstuvwxyz");
	rl_test_distribution_init(uniform_doubled, "aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz");
	t_assert(rl_distribution_compare(uniform, uniform_doubled) == 0.0f);

	rl_distribution vowels;
	rl_test_distribution_init(vowels, "aaaabcdeeeeeeefghiiiijklmnoooopqrstuuvwxyz");
	const float error_uniform_to_vowels = rl_distribution_compare(uniform, vowels);
	const float error_vowels_to_uniform = rl_distribution_compare(vowels, uniform);
	t_assert(error_uniform_to_vowels > 0.0f);
	t_assert(error_vowels_to_uniform == error_uniform_to_vowels);

	rl_distribution uniform_with_an_extra_a;
	rl_test_distribution_init(uniform_with_an_extra_a, "aabcdefghijklmnopqrstuvwxyz");
	t_assert(rl_distribution_compare(uniform, uniform_with_an_extra_a) < error_uniform_to_vowels);

	return nullptr;
}

const char* test_distribution_get_random_letter()
{
	rl_distribution distribution;
	rl_test_distribution_init(distribution, "abcdefaaaa");
	t_assert(rl_distribution_get_random_letter(distribution, 0.0f) == 'a');
	t_assert(rl_distribution_get_random_letter(distribution, 0.15f) == 'a');
	t_assert(rl_distribution_get_random_letter(distribution, 0.45f) == 'a');
	t_assert(rl_distribution_get_random_letter(distribution, 0.55f) == 'b');
	t_assert(rl_distribution_get_random_letter(distribution, 0.65f) == 'c');
	t_assert(rl_distribution_get_random_letter(distribution, 0.75f) == 'd');
	t_assert(rl_distribution_get_random_letter(distribution, 0.85f) == 'e');
	t_assert(rl_distribution_get_random_letter(distribution, 0.95f) == 'f');
	t_assert(rl_distribution_get_random_letter(distribution, 1.0f) == 'f');
	return nullptr;
}

const char* test_distribution_get_best_letters()
{
	// Create a distribution that includes all letters but favors vowels, with E being
	// the most prominent, then (in descending order) A, I, O, and U
	rl_distribution vowels;
	rl_test_distribution_init(vowels, "aaaaaaabcdeeeeeeeeeefghiiiijklmnooopqrstuuvwxyz");

	// Initialize player A's rack with one of each letter
	rl_rack rack_a;
	rl_test_rack_init(rack_a, "abcdefghijklmnopqrstuvwxyz");
	t_assert(rack_a.sum == 26);

	// Initialize player B's rack with two of each vowel and a whole lot of c's
	rl_rack rack_b;
	rl_test_rack_init(rack_b, "aaeeiioouuccccccccccccccc");
	t_assert(rack_b.sum == 25);

	// Figure out the 5 best letters to steal from player B in order to bring player A's
	// rack closest to our desired distribution
	uint8 letters[5];
	int32 num_letters = rl_distribution_get_best_letters(vowels, rack_a, rack_b, 5, letters);
	t_assert(num_letters == 5);

	// We should get two E's, two A's, and an I
	int32 num_a = 0;
	int32 num_e = 0;
	int32 num_i = 0;
	int32 num_other = 0;
	for (size_t i = 0; i < num_letters; i++)
	{
		switch (letters[i])
		{
		case 'a': num_a++; break;
		case 'e': num_e++; break;
		case 'i': num_i++; break;
		default: num_other++; break;
		}
	}
	t_assert(num_a == 2);
	t_assert(num_e == 2);
	t_assert(num_i == 1);
	t_assert(num_other == 0);

	// Input racks should not have been affected
	t_assert(rack_a.sum == 26);
	t_assert(rack_b.sum == 25);

	// Steal up to 5 from a different rack containing only 2 c's: the number of letters
	// to steal should be capped at 2, and both letters should be c
	rl_rack rack_c;
	rl_test_rack_init(rack_c, "cc");
	num_letters = rl_distribution_get_best_letters(vowels, rack_a, rack_c, 5, letters);
	t_assert(num_letters == 2);
	t_assert(letters[0] == 'c');
	t_assert(letters[1] == 'c');

	return nullptr;
}
