#pragma once

#include "rl_types.h"

struct rl_rack;

/*
	Represents the prevalence of each letter in a particular set.
*/
struct rl_distribution
{
	// Weight for each letter, with all weights summing to 1.0.
	float weights[26];
};

// Initializes a new distribution from a set of per-letter occurrences. e.g. a counts
// array of [60, 40, 0, 0, ...] and a sum of 100 will yield a distribution with weights
// weights [0.6f, 0.4f, 0.f, 0.f, ...].
void rl_distribution_init(rl_distribution& distribution, uint32 counts[26], uint32 sum);

// Initializes a new distribution from a rack of tiles
void rl_distribution_from_rack(rl_distribution& distribution, const rl_rack& rack);

// Compares two distributions, returning an absolute magnitude representing how
// significantly they differ: distributions with equal weights have an error of 0.0,
// and the more they deviate from each other, the greater the error value.
float rl_distribution_compare(const rl_distribution& lhs, const rl_distribution& rhs);

// Picks a random letter, weighted according to the given distribution, given an input
// value between 0.0 and 1.0
uint8 rl_distribution_get_random_letter(const rl_distribution& distribution, float weight);

// Finds the best possible set of letters, up to 'num' total letters, to remove from
// from_rack and add to for_rack in order to bring for_rack's letter distribution as
// close as possible to the target distribution. out_letters should be a buffer with
// enough space for up to 'num' results. Returns the number of letters that were added
// to the output buffer. Does not modify either rack.
int32 rl_distribution_get_best_letters(const rl_distribution& distribution, const rl_rack& for_rack, const rl_rack& from_rack, int32 num, uint8* out_letters);
