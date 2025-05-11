#include "rl_distribution.h"

#include <cstdlib>
#include <cstring>
#include <cfloat>

#include "rl_util.h"
#include "rl_rack.h"

static uint8 _rl_distribution_get_best_letter(const rl_distribution& distribution, rl_rack& for_rack, const rl_rack* from_rack)
{
	for_rack.sum++;

	uint8 best_char = 'a';
	float best_char_error = FLT_MAX;
	for (int32 i = 0; i < COUNT_OF(for_rack.counts); i++)
	{
		if (from_rack && from_rack->counts[i] == 0)
		{
			continue;
		}

		rl_distribution modified;
		for_rack.counts[i]++;

		rl_distribution_from_rack(modified, for_rack);
		const float error = rl_distribution_compare(distribution, modified);
		if (error < best_char_error)
		{
			best_char = 'a' + i;
			best_char_error = error;
		}

		for_rack.counts[i]--;
	}

	for_rack.sum--;
	return best_char;
}

void rl_distribution_init(rl_distribution& distribution, uint32 counts[26], uint32 sum)
{
	memset(&distribution, 0, sizeof(distribution));

	if (sum > 0)
	{
		const float total = static_cast<float>(sum);
		for (int32 i = 0; i < COUNT_OF(distribution.weights); i++)
		{
			const float count = static_cast<float>(counts[i]);
			distribution.weights[i] = count / total;
		}
	}
}
void rl_distribution_from_rack(rl_distribution& distribution, const struct rl_rack& rack)
{
	memset(&distribution, 0, sizeof(distribution));

	if (rack.sum > 0)
	{
		const float total = static_cast<float>(rack.sum);
		for (int32 i = 0; i < COUNT_OF(distribution.weights); i++)
		{
			const float count = static_cast<float>(rack.counts[i]);
			distribution.weights[i] = count / total;
		}
	}
}

float rl_distribution_compare(const rl_distribution& lhs, const rl_distribution& rhs)
{
	float error = 0.0f;
	for (int32 i = 0; i < COUNT_OF(lhs.weights); i++)
	{
		const float delta = rhs.weights[i] - lhs.weights[i];
		if (delta >= 0.0f)
		{
			error += delta;
		}
		else
		{
			error -= delta;
		}
	}
	return error;
}

uint8 rl_distribution_get_random_letter(const rl_distribution& distribution, float weight)
{
	float sum = 0.0f;
	for (int32 i = 0; i < COUNT_OF(distribution.weights); i++)
	{
		sum += distribution.weights[i];
		if (weight <= sum)
		{
			return 'a' + i;
		}
	}
	return 'z';
}

int32 rl_distribution_get_best_letters(const rl_distribution& distribution, const rl_rack& for_rack, const rl_rack& from_rack, int32 num, uint8* out_letters)
{
	rl_rack for_rack_copy;
	rl_rack from_rack_copy;
	memcpy(&for_rack_copy, &for_rack, sizeof(for_rack_copy));
	memcpy(&from_rack_copy, &from_rack, sizeof(from_rack_copy));

	int32 num_stolen = 0;
	while (from_rack_copy.sum > 0 && num_stolen < num)
	{
		const uint8 best_letter = _rl_distribution_get_best_letter(distribution, for_rack_copy, &from_rack_copy);

		rl_rack_pop(from_rack_copy, best_letter);
		rl_rack_push(for_rack_copy, best_letter);

		out_letters[num_stolen] = best_letter;
		num_stolen++;
	}
	return num_stolen;
}
