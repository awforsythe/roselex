#pragma once

#include <cstdio>
#include <cstdlib>

#include "testing.h"
#include "rl_nodelookup.h"

#include "rl_types.h"

const char* test_nodelookup_init()
{
	// Initialize a nodelookup with 4 buckets
	rl_nodelookup nodelookup;
	rl_nodelookup_init(nodelookup, 4);

	// This should allocate space for the buckets array, but the head of each linked list
	// should be null initially
	t_assert(nodelookup.capacity == 4);
	t_assert(nodelookup.buckets[0] == nullptr);
	t_assert(nodelookup.buckets[1] == nullptr);
	t_assert(nodelookup.buckets[2] == nullptr);
	t_assert(nodelookup.buckets[3] == nullptr);

	rl_nodelookup_free(nodelookup);
	return nullptr;
}

const char* test_nodelookup_insert()
{
	// Initialize a nodelookup with 4 buckets
	rl_nodelookup nodelookup;
	rl_nodelookup_init(nodelookup, 4);

	// Insert three items: 400:0xaaaa and 800:0xcccc should get slotted into bucket 0;
	// 401:0xbbbb should be at the head of bucket 1
	rl_nodelookup_insert(nodelookup, 400, 0xaaaa);
	rl_nodelookup_insert(nodelookup, 401, 0xbbbb);
	rl_nodelookup_insert(nodelookup, 800, 0xcccc);

	// The linked list in our first bucket should be 400:0xaaaa -> 800:0xcccc -> [/]
	t_assert(nodelookup.buckets[0]->signature == 400);
	t_assert(nodelookup.buckets[0]->node_index == 0xaaaa);
	t_assert(nodelookup.buckets[0]->next);
	t_assert(nodelookup.buckets[0]->next->signature == 800);
	t_assert(nodelookup.buckets[0]->next->node_index == 0xcccc);
	t_assert(!nodelookup.buckets[0]->next->next);

	// Our second bucket should have a single-item linked list, 401:0xbbbb -> [/]
	t_assert(nodelookup.buckets[1]->signature == 401);
	t_assert(nodelookup.buckets[1]->node_index == 0xbbbb);
	t_assert(!nodelookup.buckets[1]->next);

	// The remaining two buckets should still be empty
	t_assert(nodelookup.buckets[2] == nullptr);
	t_assert(nodelookup.buckets[3] == nullptr);

	rl_nodelookup_free(nodelookup);
	return nullptr;
}

const char* test_nodelookup_find()
{
	rl_nodelookup nodelookup;
	rl_nodelookup_init(nodelookup, 4);

	rl_nodelookup_insert(nodelookup, 400, 0xaaaa);
	rl_nodelookup_insert(nodelookup, 401, 0xbbbb);
	t_assert(rl_nodelookup_find(nodelookup, 400) == 0xaaaa);
	t_assert(rl_nodelookup_find(nodelookup, 401) == 0xbbbb);
	t_assert(rl_nodelookup_find(nodelookup, 800) == -1);

	rl_nodelookup_insert(nodelookup, 800, 0xcccc);
	t_assert(rl_nodelookup_find(nodelookup, 800) == 0xcccc);
	t_assert(rl_nodelookup_find(nodelookup, 999) == -1);

	rl_nodelookup_free(nodelookup);
	return nullptr;
}
