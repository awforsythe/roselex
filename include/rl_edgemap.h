#pragma once

#include "rl_types.h"

/*
	Entry in an rl_edgemap, representing a single edge leading from one DAWG node
	to another. Consider this example DAWG, which contains 6 nodes and 5 edges:

	    [root] -> (C) -> (A) -> [T] -> [S]
	                        \
	                         -> [R]

	Assume the nodes are laid out in an rl_nodearray, in this order:

	    - 0: [root]
		- 1: (C)
		- 2: (A)
		- 3: [T]
		- 4: [S]
		- 5: [R]

	Each of these 6

	The node at index 0, the root, would have an edgemap with a single edge, e.g.
	[{letter: 'a', node_index: 2}]. The node at index 2 (A) would have two edges,
	[{letter: 't', node_index: 3}, {letter: 'r', node_index: 5}]. The nodes at 4 and
	5 (S and T) would each have an empty edgemap.
*/
struct rl_edgemap_item
{
	// The letter associated with this edge, ['a'..'z']
	int32 letter;

	// Index of the node that this edge leads to in the wider rl_nodearray
	int32 node_index;
};

/*
	Associative data structure used to record the edges that form links between nodes in a
	DAWG. Must be explicitly initialized and freed.

	Storage is dynamically allocated, with the items array growing in capacity as needed.
	A DAWG may contain hundreds of thousands of nodes, with each node having a small but
	variable handful of edges: many have 0, most have a single-digit count, with 26 being
	the theoretical maximum edge count.
*/
struct rl_edgemap
{
	// Number of edges that have been added to the edgemap
	int32 size;

	// Number of edges for which space has been allocated
	int32 capacity;

	// Contiguous array of edges, indexed [0..size)
	rl_edgemap_item* items;
};

// Initializes an edgemap, initially occupying no heap memory.
void rl_edgemap_init(rl_edgemap& edgemap);

// Frees any heap memory allocated by the edgemap as it grew.
void rl_edgemap_free(rl_edgemap& edgemap);

// Inserts a new item into the edgemap, reallocating the items buffer if necessary.
void rl_edgemap_insert(rl_edgemap& edgemap, uint8 letter, int32 node_index);

// Searches for an edge associated with the given letter. If such a node exists,
// returns the index of the node to which that edge points. If no such node exists,
// returns -1.
int32 rl_edgemap_find(const rl_edgemap& edgemap, uint8 letter);

// Finds the edge associated with the given letter and overwrites its node_index,
// replacing it with the given index value. If the edgemap does not contain an item
// matching the given letter, behavior is undefined.
void rl_edgemap_replace(rl_edgemap& edgemap, uint8 letter, int32 node_index);
