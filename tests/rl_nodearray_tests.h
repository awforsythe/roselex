#pragma once

#include <cstdio>
#include <cstdlib>

#include "testing.h"
#include "rl_nodearray.h"

#include "rl_types.h"
#include "rl_edgemap.h"

const char* test_nodearray_init()
{
	rl_nodearray nodearray;
	rl_nodearray_init(nodearray, 8);

	t_assert(nodearray.capacity == 8);
	t_assert(nodearray.size == 0);
	t_assert(nodearray.items);

	rl_nodearray_free(nodearray);
	return nullptr;
}

const char* test_nodearray_push()
{
	// Initialize a nodearray and push two non-word nodes in
	rl_nodearray nodearray;
	rl_nodearray_init(nodearray, 2);

	const int32 index_a = rl_nodearray_push(nodearray, false);
	const int32 index_b = rl_nodearray_push(nodearray, false);

	// Nodes should have been added at indices 0 and 1, respectively
	t_assert(index_a == 0);
	t_assert(index_b == 1);

	// Initial capacity should hold
	t_assert(nodearray.capacity == 2);
	t_assert(nodearray.size == 2);
	t_assert(!nodearray.items[0].is_word);
	t_assert(!nodearray.items[1].is_word);

	// Push a third node, forcing a reallocation that doubles the initial capacity
	const int32 index_c = rl_nodearray_push(nodearray, true);
	t_assert(index_c == 2);

	t_assert(nodearray.capacity == 4);
	t_assert(nodearray.size == 3);
	t_assert(nodearray.items[2].is_word);

	// Any node added via rl_nodearray_push should have a next_by_letter edgemap that's
	// initially empty
	t_assert(nodearray.items[0].next_by_letter.capacity == 0);
	t_assert(nodearray.items[1].next_by_letter.capacity == 0);
	t_assert(nodearray.items[2].next_by_letter.capacity == 0);

	rl_nodearray_free(nodearray);
	return nullptr;
}

const char* test_nodearray_pop()
{
	// Init a nodearray with capacity 2, then push 3 new nodes
	rl_nodearray nodearray;
	rl_nodearray_init(nodearray, 2);
	rl_nodearray_push(nodearray, false);
	rl_nodearray_push(nodearray, false);
	rl_nodearray_push(nodearray, true);
	t_assert(nodearray.size == 3);

	// Pop the last element off the end (we must supply the index of the last item):
	// size should be decremented but allocated space will not shrink
	rl_nodearray_pop(nodearray, 2);
	t_assert(nodearray.size == 2);
	t_assert(nodearray.capacity == 4);

	// Modify the new last node so that its edgemap allocates memory
	const int32 back_index = 1;
	rl_edgemap_insert(nodearray.items[back_index].next_by_letter, 'a', 100);
	rl_edgemap_insert(nodearray.items[back_index].next_by_letter, 'e', 101);
	t_assert(nodearray.items[back_index].next_by_letter.capacity == 4);
	t_assert(nodearray.items[back_index].next_by_letter.size == 2);
	t_assert(nodearray.items[back_index].next_by_letter.items);

	// Pop this node as well: this should reset the node at index 1 in the nodearray
	// buffer, thereby deallocating the edgemap memory
	rl_nodearray_pop(nodearray, back_index);
	t_assert(nodearray.items[back_index].next_by_letter.capacity == 0);
	t_assert(nodearray.items[back_index].next_by_letter.size == 0);
	t_assert(!nodearray.items[back_index].next_by_letter.items);

	rl_nodearray_free(nodearray);
	return nullptr;
}
