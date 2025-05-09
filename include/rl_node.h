#pragma once

#include "rl_types.h"
#include "rl_edgemap.h"

/*
	A single node in the DAWG, representing a discrete position along a chain of all
	possible words formed from a given prefix. The DAWG has a single root node, with
	edges leading outward to each possible first letter, and so on. For example, a DAWG
	formed from the words "CAT", "CATS", "CAR", and "CARRY" would look like this:

	    [root] -> (C) -> (A) -> [T] -> [S]
	                        \
	                         -> [R] -> (R) -> [Y]

	This DAWG has 8 nodes in total, including the root. As an example, if we're looking
	at the the 'A' node, then the two edges leading out from node would represent the
	path toward all valid words with the prefix "CA". The 'A' node does not terminate a
	word (i.e. is_word is false, represented above by parentheses), indicating that "CA"
	is not a valid word. Following the 'T' edge to [T], we find that "CAT" is a valid
	word.

	Similarly:

	- (C) -> (A) -> [R]               => "CAR" is a valid word
	- (C) -> (A) -> [R] -> (R)        => "CARR" is not a valid word, but it is a prefix
	- (C) -> (A) -> [R] -> (R) -> [Y] => "CARRY" is a valid word

	Each node is situated as a specific offset in a large, flat rl_nodearray, meaning
	that each node can be accessed in constant time given its index in the array. Each
	node has an rl_edgemap that records the index of the next node in the DAWG, by letter.
	e.g. The 'A' node above has two edges leading out of it:

	- rl_edgemap_lookup(node.next_by_letter, 't') => returns the index of the [T] node
	- rl_edgemap_lookup(node.next_by_letter, 'r') => returns the index of the [R] node
	- rl_edgemap_lookup(node.next_by_letter, 'z') => returns -1
*/
struct rl_node
{
	// Whether this node terminates a valid word or is simply a legal prefix.
	bool is_word;

	// For any letter, records the index of the node leading out of this one with that
	// letter (if any).
	rl_edgemap next_by_letter;
};

// Initializes a node. Must call rl_node_free when done.
void rl_node_init(rl_node& node, bool is_word);

// Frees all memory in use by this node.
void rl_node_free(rl_node& node);

// Resets an rl_node struct so that it can be reused in place, deallocating and
// clearing its edgemap in the process
void rl_node_reset(rl_node& node);

// Computes a 64-bit hash for this node, as a function of the is_word flag and the full
// set of letter-to-node-index mappings in the next_by_letter edgemap, such that each
// node in a DAWG should have a unique signature
uint64 rl_node_signature(const rl_node& node);
