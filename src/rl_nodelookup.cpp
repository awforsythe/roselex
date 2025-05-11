#include "rl_nodelookup.h"

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

void rl_nodelookup_init(rl_nodelookup& nodelookup, int32 capacity)
{
	assert(capacity > 0);

	nodelookup.capacity = capacity;
	nodelookup.buckets = reinterpret_cast<rl_nodelookup_item**>(malloc(nodelookup.capacity * sizeof(rl_nodelookup_item*)));
	assert(nodelookup.buckets);

	memset(nodelookup.buckets, 0, nodelookup.capacity * sizeof(rl_nodelookup_item*));
	assert(!nodelookup.buckets[0]);
}

void rl_nodelookup_free(rl_nodelookup& nodelookup)
{
	for (int32 bucket_index = 0; bucket_index < nodelookup.capacity; bucket_index++)
	{
		rl_nodelookup_item* item = nodelookup.buckets[bucket_index];
		while (item)
		{
			rl_nodelookup_item* temp = item;
			item = item->next;
			free(temp);
		}
	}
	free(nodelookup.buckets);
}

void rl_nodelookup_insert(rl_nodelookup& nodelookup, uint64 signature, int32 node_index)
{
	const int32 bucket_index = signature % nodelookup.capacity;
	rl_nodelookup_item* item = nodelookup.buckets[bucket_index];

	rl_nodelookup_item* base = nullptr;
	while (item)
	{
		if (item->signature == signature)
		{
			printf("WARNING: Duplicate node with signature %" PRIu64 "\n", signature);
			return;
		}
		base = item;
		item = item->next;
	}

	item = reinterpret_cast<rl_nodelookup_item*>(malloc(sizeof(rl_nodelookup_item)));
	item->next = nullptr;
	item->signature = signature;
	item->node_index = node_index;
	if (base)
	{
		assert(!base->next);
		base->next = item;
	}
	else
	{
		nodelookup.buckets[bucket_index] = item;
	}
}

int32 rl_nodelookup_find(const rl_nodelookup& nodelookup, uint64 signature)
{
	const int32 bucket_index = signature % nodelookup.capacity;
	rl_nodelookup_item* item = nodelookup.buckets[bucket_index];
	while (item)
	{
		if (item->signature == signature)
		{
			return item->node_index;
		}
		item = item->next;
	}
	return -1;
}
