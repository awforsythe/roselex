#pragma once

#include "rl_types.h"

struct rl_node;

/*
	A one-dimensional array of nodes that represent the data in a DAWG. Wraps a
	fixed-size buffer that will be reallocated if the array grows beyond its initial
	capacity.
*/
struct rl_nodearray
{
	// Contiguous array of rl_node structs, indexed [0..size)
	rl_node* items;
	
	// Number of items for which memory has been allocated
	int32 capacity;

	// Number of valid items stored in the array
	int32 size;
};

// Initializes a new nodearray, allocating a buffer with the desired initial capacity,
// which must be >0. You must call rl_nodearray_free when finished with the nodearray.
void rl_nodearray_init(rl_nodearray& nodearray, int32 capacity);

// Frees all memory allocated for use by this nodearray.
void rl_nodearray_free(rl_nodearray& nodearray);

// Adds a new rl_node item into the buffer, reallocating the array if at capacity.
int32 rl_nodearray_push(rl_nodearray& nodearray, bool is_word);

// Removes the last item from the buffer, shrinking the size of the array by 1. Caller
// must pass the index of the last item. If the provided index is not equal to the index
// of the last item, behavior is undefined.
void rl_nodearray_pop(rl_nodearray& nodearray, int32 back_index);
