#include "rl_edgemap.h"

#include <cstdlib>
#include <cstring>
#include <cassert>

static rl_edgemap_item* _rl_edgemap_bsearch(const rl_edgemap& edgemap, uint8 letter)
{
	rl_edgemap_item* base = edgemap.items;
	for (int32 lim = edgemap.size; lim != 0; lim >>= 1)
	{
		rl_edgemap_item* item = base + (lim >> 1);
		const int32 cmp = letter - item->letter;
		if (cmp == 0)
		{
			return item;
		}
		if (cmp > 0)
		{
			base = item + 1;
			lim--;
		}
	}
	return nullptr;
}

void rl_edgemap_init(rl_edgemap& edgemap)
{
	edgemap.size = 0;
	edgemap.capacity = 0;
	edgemap.items = nullptr;
}

void rl_edgemap_free(rl_edgemap& edgemap)
{
	free(edgemap.items);
}

void rl_edgemap_insert(rl_edgemap& edgemap, uint8 letter, int32 node_index)
{
	if (edgemap.size == edgemap.capacity)
	{
		edgemap.capacity += 4;
		rl_edgemap_item* new_items = reinterpret_cast<rl_edgemap_item*>(realloc(edgemap.items, edgemap.capacity * sizeof(rl_edgemap_item)));
		assert(new_items);
		edgemap.items = new_items;
	}

	const int32 new_index = edgemap.size;
	assert(new_index == 0 || letter > edgemap.items[new_index - 1].letter);
	rl_edgemap_item& item = edgemap.items[new_index];
	item.letter = letter;
	item.node_index = node_index;
	edgemap.size++;
}

int32 rl_edgemap_find(const rl_edgemap& edgemap, uint8 letter)
{
	const rl_edgemap_item* found = _rl_edgemap_bsearch(edgemap, letter);
	if (found)
	{
		return found->node_index;
	}
	return -1;
}

void rl_edgemap_replace(rl_edgemap& edgemap, uint8 letter, int32 node_index)
{
	rl_edgemap_item* found = _rl_edgemap_bsearch(edgemap, letter);
	assert(found);
	found->node_index = node_index;
}
