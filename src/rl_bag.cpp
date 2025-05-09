#include "rl_bag.h"

#include <cstdlib>
#include <cstring>
#include <cassert>

#include "rl_util.h"

static int32 RL_DEFAULT_WEIGHTS[COUNT_OF(rl_bag::weights)] = { 812,149,271,432,1202,230,203,592,731,010,69,398,261,695,768,182,011,602,628,910,288,111,209,17,211,7 };

void rl_bag_init(rl_bag& bag)
{
	bag.weight_sum = 0;
	for (uint8 ordinal = 0; ordinal < COUNT_OF(bag.weights); ordinal++)
	{
		bag.weights[ordinal] = RL_DEFAULT_WEIGHTS[ordinal];
		bag.weight_sum += bag.weights[ordinal];
	}
}

uint8 rl_bag_draw(const rl_bag& bag)
{
	int32 randval = rand() % bag.weight_sum;
	for (uint8 ordinal = 0; ordinal < COUNT_OF(bag.weights); ordinal++)
	{
		if (randval < bag.weights[ordinal])
		{
			return 'a' + ordinal;
		}
		randval -= bag.weights[ordinal];
	}
	assert(false && "randval should never exceed bag.weight_sum");
	return 'a';
}
