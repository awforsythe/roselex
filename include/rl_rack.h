#pragma once

#include "rl_types.h"

/*
	A collection of letter tiles from which moves can be formed.
*/
struct rl_rack
{
	// Number of occurrences of each letter, starting at index 0 for 'a'
	uint8 counts[32];

	// Total number of letter tiles held in the rack
	int32 sum;
};

// Initializes a rack, setting its letter counts to zero.
void rl_rack_init(rl_rack& rack);

// Adds a single letter ('a' through 'z') to the given rack.
void rl_rack_push(rl_rack& rack, uint8 letter);

// Returns whether the rack has at least one of the given letter.
bool rl_rack_find(const rl_rack& rack, uint8 letter);

// Attempts to remove a single instance of a letter from the specified rack, provided
// it has at least one. Returns success.
bool rl_rack_pop(rl_rack& rack, uint8 letter);

// Removes multiple letters from the given rack,
void rl_rack_subtract(rl_rack& rack, const rl_rack& other);
