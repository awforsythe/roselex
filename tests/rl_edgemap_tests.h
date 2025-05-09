#pragma once

#include <cstdio>
#include <cstdlib>

#include "testing.h"
#include "rl_edgemap.h"

#include "rl_types.h"

const char* test_edgemap_init()
{
	rl_edgemap edgemap;
	rl_edgemap_init(edgemap);

	t_assert(edgemap.capacity == 0);
	t_assert(edgemap.size == 0);
	t_assert(!edgemap.items);

	rl_edgemap_free(edgemap);
	return nullptr;
}

const char* test_edgemap_insert()
{
	rl_edgemap edgemap;
	rl_edgemap_init(edgemap);

	rl_edgemap_insert(edgemap, 'a', 0xaaaa);
	rl_edgemap_insert(edgemap, 'b', 0xbbbb);
	rl_edgemap_insert(edgemap, 'c', 0xcccc);

	t_assert(edgemap.capacity == 4);
	t_assert(edgemap.size == 3);
	t_assert(edgemap.items);

	t_assert(edgemap.items[0].letter == 'a');
	t_assert(edgemap.items[0].node_index == 0xaaaa);
	t_assert(edgemap.items[1].letter == 'b');
	t_assert(edgemap.items[1].node_index == 0xbbbb);
	t_assert(edgemap.items[2].letter == 'c');
	t_assert(edgemap.items[2].node_index == 0xcccc);

	rl_edgemap_free(edgemap);
	return nullptr;
}

const char* test_edgemap_find()
{
	rl_edgemap edgemap;
	rl_edgemap_init(edgemap);

	t_assert(rl_edgemap_find(edgemap, 'a') == -1);
	rl_edgemap_insert(edgemap, 'a', 42);
	t_assert(rl_edgemap_find(edgemap, 'a') == 42);

	rl_edgemap_free(edgemap);
	return nullptr;
}

const char* test_edgemap_replace()
{
	rl_edgemap edgemap;
	rl_edgemap_init(edgemap);

	t_assert(rl_edgemap_find(edgemap, 'a') == -1);
	rl_edgemap_insert(edgemap, 'a', 42);
	t_assert(rl_edgemap_find(edgemap, 'a') == 42);
	rl_edgemap_replace(edgemap, 'a', 100);
	t_assert(rl_edgemap_find(edgemap, 'a') == 100);

	rl_edgemap_free(edgemap);
	return nullptr;
}
