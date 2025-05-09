#include "rl_rack.h"

#include <cassert>
#include <cstring>

#include "rl_util.h"

int32 _rl_rack_index(const rl_rack& rack, uint8 letter)
{
	const int32 index = letter - 'a';
	assert(index >= 0 && index < COUNT_OF(rack.counts));
	return index;
}

void rl_rack_init(rl_rack& rack)
{
	memset(rack.counts, 0, sizeof(rack.counts));
	rack.sum = 0;
}

void rl_rack_push(rl_rack& rack, uint8 letter)
{
	const int32 index = _rl_rack_index(rack, letter);
	if (rack.counts[index] < UINT8_MAX)
	{
		rack.counts[index]++;
		rack.sum++;
	}
}

bool rl_rack_find(const rl_rack& rack, uint8 letter)
{
	const int32 index = _rl_rack_index(rack, letter);
	return rack.counts[index] > 0;
}

bool rl_rack_pop(rl_rack& rack, uint8 letter)
{
	const int32 index = _rl_rack_index(rack, letter);
	if (rack.counts[index] > 0)
	{
		rack.sum--;
		rack.counts[index]--;
		return true;
	}
	return false;
}

void rl_rack_subtract(rl_rack& rack, const rl_rack& other)
{
	for (int32 index = 0; index < COUNT_OF(rack.counts); index++)
	{
		int32 other_count = other.counts[index];
		while (other_count > 0 && rack.counts[index] > 0)
		{
			rack.counts[index]--;
			rack.sum--;
			other_count--;
		}
	}
}
