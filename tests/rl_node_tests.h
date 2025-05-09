#pragma once

#include <cstdio>
#include <cstdlib>

#include "testing.h"
#include "rl_node.h"

#include "rl_types.h"
#include "rl_edgemap.h"

const char* test_node_init()
{
	const int32 root_index = 0;
	const int32 a_index = 1;
	const int32 at_index = 2;

	rl_node root; rl_node_init(root, false);
	rl_node a; rl_node_init(a, false);
	rl_node at; rl_node_init(at, true);

	rl_edgemap_insert(root.next_by_letter, 'a', a_index);
	rl_edgemap_insert(a.next_by_letter, 't', at_index);

	t_assert(!root.is_word);
	t_assert(!a.is_word);
	t_assert(at.is_word);

	t_assert(root.next_by_letter.size == 1);
	t_assert(root.next_by_letter.items[0].letter == 'a');
	t_assert(root.next_by_letter.items[0].node_index == a_index);
	t_assert(a.next_by_letter.size == 1);
	t_assert(a.next_by_letter.items[0].letter == 't');
	t_assert(a.next_by_letter.items[0].node_index == at_index);
	t_assert(at.next_by_letter.size == 0);

	rl_node_free(root);
	rl_node_free(a);
	rl_node_free(at);

	return nullptr;
}

const char* test_node_reset()
{
	// Initialize a node: it should have no edges initially, and no memory reserved
	rl_node node;
	rl_node_init(node, true);

	t_assert(node.is_word);
	t_assert(node.next_by_letter.size == 0);
	t_assert(node.next_by_letter.capacity == 0);
	t_assert(!node.next_by_letter.items);

	// Populating the edge map should resize the edgemap
	rl_edgemap_insert(node.next_by_letter, 'a', 1);
	rl_edgemap_insert(node.next_by_letter, 'b', 2);
	rl_edgemap_insert(node.next_by_letter, 'c', 3);
	rl_edgemap_insert(node.next_by_letter, 'd', 4);
	rl_edgemap_insert(node.next_by_letter, 'e', 5);

	t_assert(node.next_by_letter.size == 5);
	t_assert(node.next_by_letter.capacity >= 5);
	t_assert(node.next_by_letter.items);

	// Resetting the node should clear the edgemap and the is_word flag
	rl_node_reset(node);

	t_assert(!node.is_word);
	t_assert(node.next_by_letter.size == 0);
	t_assert(node.next_by_letter.capacity == 0);
	t_assert(!node.next_by_letter.items);

	rl_node_free(node);

	return nullptr;
}

const char* test_node_signature()
{
	rl_node node;
	rl_node_init(node, false);

	// Validate an initial hash with is_word: false and no edges
	uint64_t sig;
	sig = rl_node_signature(node); t_assert(sig == 0xaf63bd4c8601b7df);

	// Inserting an edge should affect the hash
	rl_edgemap_insert(node.next_by_letter, 'a', 12);
	sig = rl_node_signature(node); t_assert(sig == 0x67f0c9abb132c4b2);

	// An edge with the same letter but a different index should have a different hash
	rl_node_reset(node);
	rl_edgemap_insert(node.next_by_letter, 'a', 13);
	sig = rl_node_signature(node); t_assert(sig == 0x7eb75b407688143);

	// Setting the is_word flag should change the hash
	node.is_word = true;
	sig = rl_node_signature(node); t_assert(sig == 0x3133189a98132e90);

	rl_node_free(node);

	return nullptr;
}
