#include "rl_dawg.h"

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

#include "rl_util.h"
#include "rl_node.h"

static void _rl_dawg_ctx_minimize(rl_dawg_ctx& ctx, int32 to_depth)
{
	while (ctx.edge_stack_size > to_depth)
	{
		rl_pending_edge& edge = ctx.edge_stack[ctx.edge_stack_size - 1];
		assert(edge.from_index >= 0 && edge.from_index < ctx.nodearray.size);
		assert(edge.to_index >= 0 && edge.to_index < ctx.nodearray.size);

		rl_node& to_node = ctx.nodearray.items[edge.to_index];
		const uint64 to_node_signature = rl_node_signature(to_node);
		const int32 equivalent_node_index = rl_nodelookup_find(ctx.minimized_lookup, to_node_signature);
		if (equivalent_node_index >= 0)
		{
			rl_node& from_node = ctx.nodearray.items[edge.from_index];
			rl_edgemap_replace(from_node.next_by_letter, edge.letter, equivalent_node_index);
			rl_nodearray_pop(ctx.nodearray, edge.to_index);
		}
		else
		{
			rl_nodelookup_insert(ctx.minimized_lookup, to_node_signature, edge.to_index);
		}
		ctx.edge_stack_size--;
	}
}

void rl_dawg_ctx_init(rl_dawg_ctx& ctx)
{
	rl_nodearray_init(ctx.nodearray, 8192);
	rl_nodearray_push(ctx.nodearray, false);
	rl_nodelookup_init(ctx.minimized_lookup, 8192);

	memset(ctx.edge_stack, 0, sizeof(ctx.edge_stack));
	ctx.edge_stack_size = 0;

	ctx.prev_word_len = 0;

	memset(ctx.letter_counts, 0, sizeof(ctx.letter_counts));
	ctx.letter_counts_sum = 0;
}

void rl_dawg_ctx_free(rl_dawg_ctx& ctx)
{
	rl_nodearray_free(ctx.nodearray);
	rl_nodelookup_free(ctx.minimized_lookup);
}

bool rl_dawg_ctx_add(rl_dawg_ctx& ctx, const uint8* word, int32 word_len)
{
	// Abort if we were called without a valid word: let's be permissive and not just crash, since this could potentially be exposed to user input
	if (!word || word_len == 0)
	{
		return false;
	}

	// Reject any buffer-bustin' words that exceed our maximum length
	if (word_len > RL_MAX_WORD_LEN)
	{
		return false;
	}

	// Require that every character in the word is a simple ASCII letter 'a' thru 'z'
	for (int32 letter_index = 0; letter_index < word_len; letter_index++)
	{
		const uint8 letter = word[letter_index];
		if (letter < 'a' || letter > 'z')
		{
			return false;
		}
	}

	// Count each letter in the word toward our base distribution
	for (int32 letter_index = 0; letter_index < word_len; letter_index++)
	{
		const uint8 letter = word[letter_index];
		const uint8 index = letter - 'a';

		ctx.letter_counts[index]++;
		ctx.letter_counts_sum++;
	}

	// Enforce that all words must be added in alphabetical order
	const int32 min_word_len = MIN(ctx.prev_word_len, word_len);
	if (min_word_len > 0)
	{
		const int32 cmp = memcmp(ctx.prev_word, word, min_word_len);
		if (cmp > 0 || (cmp == 0 && word_len < ctx.prev_word_len))
		{
			printf("REJECT: %.*s\n", word_len, word);
			return false;
		}
	}

	int32 common_prefix_depth = 0;
	while (common_prefix_depth < min_word_len)
	{
		if (ctx.prev_word[common_prefix_depth] != word[common_prefix_depth])
		{
			break;
		}
		common_prefix_depth++;
	}

	if (common_prefix_depth < min_word_len)
	{
		_rl_dawg_ctx_minimize(ctx, common_prefix_depth);
	}

	int32 prev_node_index = ctx.edge_stack_size > 0 ? ctx.edge_stack[ctx.edge_stack_size - 1].to_index : 0;
	for (int32 letter_index = common_prefix_depth; letter_index < word_len; letter_index++)
	{
		const uint8 letter = word[letter_index];
		const bool is_word = letter_index == word_len - 1;

		const int32 new_node_index = rl_nodearray_push(ctx.nodearray, is_word);
		rl_node& prev_node = ctx.nodearray.items[prev_node_index];
		rl_edgemap_insert(prev_node.next_by_letter, letter, new_node_index);

		assert(ctx.edge_stack_size < COUNT_OF(ctx.edge_stack));
		rl_pending_edge& edge = ctx.edge_stack[ctx.edge_stack_size];
		edge.from_index = prev_node_index;
		edge.to_index = new_node_index;
		edge.letter = letter;
		ctx.edge_stack_size++;

		prev_node_index = new_node_index;
	}

	memcpy(ctx.prev_word, word, word_len);
	ctx.prev_word_len = word_len;
	return true;
}

void rl_dawg_ctx_finalize(rl_dawg_ctx& ctx)
{
	_rl_dawg_ctx_minimize(ctx, 0);
}

void rl_dawg_ctx_move_nodes(rl_dawg_ctx& ctx, rl_dawg& dawg)
{
	// Transfer ownership of the DAWG context's nodearray to the dawg
	dawg.nodearray.capacity = ctx.nodearray.capacity;
	dawg.nodearray.size = ctx.nodearray.size;
	dawg.nodearray.items = ctx.nodearray.items;

	ctx.nodearray.capacity = 0;
	ctx.nodearray.size = 0;
	ctx.nodearray.items = nullptr;
}

void rl_dawg_init(rl_dawg& dawg)
{
	memset(&dawg, 0, sizeof(dawg));
}

void rl_dawg_free(rl_dawg& dawg)
{
	rl_nodearray_free(dawg.nodearray);
}

int32 rl_dawg_build(rl_dawg& dawg, const char* wordlist_path)
{
	assert(dawg.nodearray.capacity == 0);
	assert(dawg.nodearray.size == 0);
	assert(!dawg.nodearray.items);

	// Open the word list file: it should be a list of whitespace-delimited words, in lexicographical order
	FILE* fp = fopen(wordlist_path, "r");
	if (!fp)
	{
		printf("WARNING: Could not open '%s' for read\n", wordlist_path);
		return 0;
	}

	// Initialize a context object to contain the state necessary for building the DAWG
	rl_dawg_ctx ctx;
	rl_dawg_ctx_init(ctx);

	// Read the file word-by-word, and feed each word into the DAWG context
	int32 num_words_accepted = 0;
	char temp[512];
	while (fscanf(fp, "%511s", temp) == 1)
	{
		const uint8* word = reinterpret_cast<uint8*>(temp);
		const int32 word_len = static_cast<int32>(strlen(temp));
		if (rl_dawg_ctx_add(ctx, word, word_len))
		{
			num_words_accepted++;
		}
	}
	fclose(fp);

	// Compute the frequency with which each letter appears in the input word list
	rl_distribution_init(dawg.distribution, ctx.letter_counts, ctx.letter_counts_sum);

	// Finish minimizing the DAWG, transfer ownership of the node aray into the rl_dawg, then free the context
	rl_dawg_ctx_finalize(ctx);
	rl_dawg_ctx_move_nodes(ctx, dawg);
	rl_dawg_ctx_free(ctx);
	return num_words_accepted;
}
