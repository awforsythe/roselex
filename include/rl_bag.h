#pragma once

#include "rl_types.h"

struct rl_bag
{
	int32 weights[26];
	int32 weight_sum;
};

void rl_bag_init(rl_bag& bag);
uint8 rl_bag_draw(const rl_bag& bag);
