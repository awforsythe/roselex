#include "rl_nodearray.h"

#include <cstdlib>
#include <cassert>

#include "rl_node.h"

void rl_nodearray_init(rl_nodearray& nodearray, int32 capacity)
{
	assert(capacity > 0);

	nodearray.capacity = capacity;
	nodearray.size = 0;
	nodearray.items = reinterpret_cast<rl_node*>(malloc(nodearray.capacity * sizeof(rl_node)));

	assert(nodearray.items);
}

void rl_nodearray_free(rl_nodearray& nodearray)
{
	for (int32 node_index = 0; node_index < nodearray.size; node_index++)
	{
		rl_node& node = nodearray.items[node_index];
		rl_node_free(node);
	}
	free(nodearray.items);
}

int32 rl_nodearray_push(rl_nodearray& nodearray, bool is_word)
{
	if (nodearray.size == nodearray.capacity)
	{
		nodearray.capacity *= 2;
		rl_node* new_items = reinterpret_cast<rl_node*>(realloc(nodearray.items, nodearray.capacity * sizeof(rl_node)));
		assert(new_items);
		nodearray.items = new_items;
	}

	const int32 new_index = nodearray.size;
	rl_node& node = nodearray.items[new_index];
	rl_node_init(node, is_word);
	nodearray.size++;
	return new_index;
}

void rl_nodearray_pop(rl_nodearray& nodearray, int32 back_index)
{
	assert(back_index == nodearray.size - 1);
	rl_node_reset(nodearray.items[nodearray.size - 1]);
	nodearray.size--;
}
