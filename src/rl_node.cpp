#include "rl_node.h"

#include "rl_util.h"

#include <cassert>

static const uint64 _FNV_PRIME = 0x100000001b3;
static const uint64 _FNV_OFFSET_BASIS = 0xcbf29ce484222325;

#pragma warning (push)
#pragma warning (disable: 4307)
static const uint64 _FNV_START_ZERO = (_FNV_OFFSET_BASIS ^ 0) * _FNV_PRIME;
static const uint64 _FNV_START_ONE = (_FNV_OFFSET_BASIS ^ 1) * _FNV_PRIME;
#pragma warning (pop)

static uint64 _fnv1a(uint64 start, const uint8* buf, int32 size)
{
	uint64 hash = start;
	for (int32 i = 0; i < size; i++)
	{
		hash ^= buf[i];
		hash *= _FNV_PRIME;
	}
	return hash;
}

void rl_node_init(rl_node& node, bool is_word)
{
	node.is_word = is_word;
	rl_edgemap_init(node.next_by_letter);
}

void rl_node_free(rl_node& node)
{
	rl_edgemap_free(node.next_by_letter);
}

void rl_node_reset(rl_node& node)
{
	node.is_word = false;
	rl_edgemap_free(node.next_by_letter);
	rl_edgemap_init(node.next_by_letter);
}

uint64 rl_node_signature(const rl_node& node)
{
	const uint64 start = node.is_word ? _FNV_START_ONE : _FNV_START_ZERO;
	const uint8* edgemap_bytes = reinterpret_cast<uint8*>(node.next_by_letter.items);
	const int32 edgemap_bytes_size = node.next_by_letter.size * sizeof(rl_edgemap_item);
	return _fnv1a(start, edgemap_bytes, edgemap_bytes_size);
}
