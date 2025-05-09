#pragma once

#include "rl_types.h"

/*
	Item in a linked list that records the signature of a given node along with its
	index.
*/
struct rl_nodelookup_item
{
	// Pointer to the next item in the linked list, or nullptr if this item is the last
	rl_nodelookup_item* next;

	// Exact signature (i.e. hash) of the node in question
	uint64 signature;

	// Index at which that node can be found in the original nodearray
	int32 node_index;
};

/*
	Hash map used to facilitate fast lookups of a given node's index in the nodearray,
	given the unique signature computed from that node. Used in the process of
	finalizing the DAWG, letting us quickly identify if a particular node is identical
	to one we've already seen before.
*/
struct rl_nodelookup
{
	// Number of buckets allocated: node signatures are modulo'd by this capacity to
	// determine the bucket in which they belong. Capacity never changes once the
	// nodelookup is initialized.
	int32 capacity;

	// Array of buckets: buckets[i] is a pointer to the first rl_nodelookup_item in a
	// linked list that contains all nodes which hash to that bucket.
	rl_nodelookup_item** buckets;
};

// Initializes a new nodelookup with the given number of buckets, each of which is the
// head of a linked list. You must call rl_nodelookup_free when done with the
// nodelookup.
void rl_nodelookup_init(rl_nodelookup& nodelookup, int32 capacity);

// Frees all memory allocated by the nodelookup, traversing the linked list in each
// bucket to ensure that all items are freed.
void rl_nodelookup_free(rl_nodelookup& nodelookup);

// Inserts an item into the lookup, given its hash and the node_index value to
// associate with that hash. In the event of a hash collision, ignores the new item,
// silently refusing to insert it into the lookup.
void rl_nodelookup_insert(rl_nodelookup& nodelookup, uint64 signature, int32 node_index);

// Searches for an item that was previously inserted using the given hash. If such an
// item exists, returns the node_index that was stored. Otherwise, returns -1.
int32 rl_nodelookup_find(const rl_nodelookup& nodelookup, uint64 signature);
