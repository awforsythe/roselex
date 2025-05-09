#pragma once

#include <cstdio>
#include <cstring>

#include "testing.h"
#include "rl_testing.h"
#include "rl_dawg.h"

#include "rl_types.h"
#include "rl_util.h"
#include "rl_node.h"
#include "rl_edgemap.h"

#define t_word(s) reinterpret_cast<const uint8*>(s)

const char* test_dawg_ctx_init()
{
	// Initialize a dawg context, used internally by rl_dawg_build (must be freed)
	rl_dawg_ctx ctx;
	rl_dawg_ctx_init(ctx);

	t_assert(ctx.nodearray.capacity > 0);
	t_assert(ctx.nodearray.size == 1);
	t_assert(!ctx.nodearray.items[0].is_word);
	t_assert(ctx.nodearray.items[0].next_by_letter.capacity == 0);
	t_assert(ctx.nodearray.items[0].next_by_letter.size == 0);
	t_assert(!ctx.nodearray.items[0].next_by_letter.items);

	t_assert(ctx.minimized_lookup.capacity > 0);
	t_assert(ctx.minimized_lookup.buckets);

	t_assert(ctx.edge_stack_size == 0);
	t_assert(ctx.prev_word_len == 0);
	for (size_t i = 0; i < COUNT_OF(ctx.letter_counts); i++)
	{
		t_assert(ctx.letter_counts[i] == 0);
	}
	t_assert(ctx.letter_counts_sum == 0);

	rl_dawg_ctx_free(ctx);
	return nullptr;
}

const char* test_dawg_ctx_add()
{
	// Initialize a dawg content, used internally by rl_dawg_build (must be freed)
	rl_dawg_ctx ctx;
	rl_dawg_ctx_init(ctx);

	// Plain-lowercase-ASCII words added in alphabetical order should be accepted
	bool ok;
	ok = rl_dawg_ctx_add(ctx, t_word("bat"), 3); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("cat"), 3); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("catalyst"), 8); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("cater"), 5); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("catering"), 8); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("caterwaul"), 9); t_assert(ok);

	// A valid word added out of order should be rejected
	t_assert(ctx.prev_word_len == 9);
	t_assert(memcmp(ctx.prev_word, t_word("caterwaul"), 9) == 0);
	ok = rl_dawg_ctx_add(ctx, t_word("apple"), 5); t_assert(!ok);

	// Invalid word values should be rejected
	ok = rl_dawg_ctx_add(ctx, nullptr, 0); t_assert(!ok);
	ok = rl_dawg_ctx_add(ctx, t_word(""), 0); t_assert(!ok);
	ok = rl_dawg_ctx_add(ctx, t_word("inval!d"), 7); t_assert(!ok);
	ok = rl_dawg_ctx_add(ctx, t_word("averylongwordthatissignificantlylongerthanthemaximumlimitforaword"), 65); t_assert(!ok);

	// The root of our DAWG should have two edges leading out: one for 'b' and one for 'c'
	const rl_node& root = ctx.nodearray.items[0];
	t_assert(!root.is_word);
	t_assert(root.next_by_letter.size == 2);

	// Following the 'b' edge, we should have:
	// [root] -> b -> a -> T
	const int32 node_b_index = rl_edgemap_find(root.next_by_letter, 'b');
	t_assert(node_b_index >= 0);
	const rl_node& node_b = ctx.nodearray.items[node_b_index];
	t_assert(!node_b.is_word);
	t_assert(node_b.next_by_letter.size == 1);
	t_assert(node_b.next_by_letter.items[0].letter == 'a');
	const rl_node& node_ba = ctx.nodearray.items[node_b.next_by_letter.items[0].node_index];
	t_assert(!node_ba.is_word);
	t_assert(node_ba.next_by_letter.size == 1);
	t_assert(node_ba.next_by_letter.items[0].letter == 't');
	const rl_node& node_bat = ctx.nodearray.items[node_ba.next_by_letter.items[0].node_index];
	t_assert(node_bat.is_word);
	t_assert(node_bat.next_by_letter.size == 0);

	// Following the 'c' edge, we should have
	// [root] -> c -> a -> T -> a -> l -> y -> s -> T
	//                      \
	//                       -> e -> R -> i -> n -> G
	//                                \
	//                                 -> w -> a -> u -> L
	const rl_node& node_c = ctx.nodearray.items[rl_edgemap_find(root.next_by_letter, 'c')];
	const rl_node& node_ca = ctx.nodearray.items[rl_edgemap_find(node_c.next_by_letter, 'a')];
	const rl_node& node_cat = ctx.nodearray.items[rl_edgemap_find(node_ca.next_by_letter, 't')];
	const rl_node& node_cata = ctx.nodearray.items[rl_edgemap_find(node_cat.next_by_letter, 'a')];
	const rl_node& node_catal = ctx.nodearray.items[rl_edgemap_find(node_cata.next_by_letter, 'l')];
	const rl_node& node_cataly = ctx.nodearray.items[rl_edgemap_find(node_catal.next_by_letter, 'y')];
	const rl_node& node_catalys = ctx.nodearray.items[rl_edgemap_find(node_cataly.next_by_letter, 's')];
	const rl_node& node_catalyst = ctx.nodearray.items[rl_edgemap_find(node_catalys.next_by_letter, 't')];
	const rl_node& node_cate = ctx.nodearray.items[rl_edgemap_find(node_cat.next_by_letter, 'e')];
	const rl_node& node_cater = ctx.nodearray.items[rl_edgemap_find(node_cate.next_by_letter, 'r')];
	const rl_node& node_cateri = ctx.nodearray.items[rl_edgemap_find(node_cater.next_by_letter, 'i')];
	const rl_node& node_caterin = ctx.nodearray.items[rl_edgemap_find(node_cateri.next_by_letter, 'n')];
	const rl_node& node_catering = ctx.nodearray.items[rl_edgemap_find(node_caterin.next_by_letter, 'g')];
	const rl_node& node_caterw = ctx.nodearray.items[rl_edgemap_find(node_cater.next_by_letter, 'w')];
	const rl_node& node_caterwa = ctx.nodearray.items[rl_edgemap_find(node_caterw.next_by_letter, 'a')];
	const rl_node& node_caterwau = ctx.nodearray.items[rl_edgemap_find(node_caterwa.next_by_letter, 'u')];
	const rl_node& node_caterwaul = ctx.nodearray.items[rl_edgemap_find(node_caterwau.next_by_letter, 'l')];
	t_assert(!node_c.is_word);
	t_assert(!node_ca.is_word);
	t_assert(node_cat.is_word);
	t_assert(!node_cata.is_word);
	t_assert(!node_catal.is_word);
	t_assert(!node_cataly.is_word);
	t_assert(!node_catalys.is_word);
	t_assert(node_catalyst.is_word);
	t_assert(!node_cate.is_word);
	t_assert(node_cater.is_word);
	t_assert(!node_cateri.is_word);
	t_assert(!node_caterin.is_word);
	t_assert(node_catering.is_word);
	t_assert(!node_caterw.is_word);
	t_assert(!node_caterwa.is_word);
	t_assert(!node_caterwau.is_word);
	t_assert(node_caterwaul.is_word);

	rl_dawg_ctx_free(ctx);
	return nullptr;
}

const char* test_dawg_ctx_finalize()
{
	rl_dawg_ctx ctx;
	rl_dawg_ctx_init(ctx);
	bool ok;

	// Build up an initial DAWG with these nodes/edges:
	// [0] -c-> [1] -a-> [2] -t-> [3] -s-> [4]
	//  |
	//  f-> [5] -a-> [6] -c-> [7] -e-> [8] -t-> [9] -s-> [10]
	ok = rl_dawg_ctx_add(ctx, t_word("cat"), 3); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("cats"), 4); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("facet"), 5); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("facets"), 6); t_assert(ok);

	// We should have 11 total nodes: root + len('cats') + len('facets')
	t_assert(ctx.nodearray.size == 11);

	// The root node should branch out with 'c' and 'f' edges, to two distinct branches
	t_assert(ctx.nodearray.items[0].next_by_letter.items[0].letter == 'c');
	t_assert(ctx.nodearray.items[0].next_by_letter.items[0].node_index == 1);
	t_assert(ctx.nodearray.items[0].next_by_letter.items[1].letter == 'f');
	t_assert(ctx.nodearray.items[0].next_by_letter.items[1].node_index == 5);

	// 'caTS' and 'faceTS' have the 'TS' suffix in common, but they should not have
	// been minimized yet: if we follow 'cat' from the root, we should end up at node
	// 3, but following 'facet' from the root should bring us to node 9, on an entirely
	// separate branch
	{
		const rl_node& node_c = ctx.nodearray.items[rl_edgemap_find(ctx.nodearray.items[0].next_by_letter, 'c')];
		const rl_node& node_ca = ctx.nodearray.items[rl_edgemap_find(node_c.next_by_letter, 'a')];
		const int32 node_index_cat = rl_edgemap_find(node_ca.next_by_letter, 't');
		t_assert(node_index_cat == 3);

		const rl_node& node_f = ctx.nodearray.items[rl_edgemap_find(ctx.nodearray.items[0].next_by_letter, 'f')];
		const rl_node& node_fa = ctx.nodearray.items[rl_edgemap_find(node_f.next_by_letter, 'a')];
		const rl_node& node_fac = ctx.nodearray.items[rl_edgemap_find(node_fa.next_by_letter, 'c')];
		const rl_node& node_face = ctx.nodearray.items[rl_edgemap_find(node_fac.next_by_letter, 'e')];
		const int32 node_index_facet = rl_edgemap_find(node_face.next_by_letter, 't');
		t_assert(node_index_facet == 9);	
	}

	// Now add 'fact': before the DAWG is updated, we should minimize all
	// previously-added branches that don't share a common prefix wit this word, and
	// then a new 't' suffix should be tacked on to 'fac', giving us:
	// [0] -c-> [1] -a-> [2] -t-> [3] -s-> [4]
	//  |                  \e
	//  |                   \
	//  f-> [5] -a-> [6] -c-> [7] -t-> [8]
	ok = rl_dawg_ctx_add(ctx, t_word("fact"), 4); t_assert(ok);
	t_assert(ctx.nodearray.size == 9);

	// 'caTS' and 'faceTS' should now converge to a common 'TS' suffix branch:
	// following 'cat' and 'facet' from the root should now lead us to the same node,
	// since both words share the same set of suffixes
	{
		const rl_node& node_c = ctx.nodearray.items[rl_edgemap_find(ctx.nodearray.items[0].next_by_letter, 'c')];
		const rl_node& node_ca = ctx.nodearray.items[rl_edgemap_find(node_c.next_by_letter, 'a')];
		const int32 node_index_cat = rl_edgemap_find(node_ca.next_by_letter, 't');
		t_assert(node_index_cat == 3);

		const rl_node& node_f = ctx.nodearray.items[rl_edgemap_find(ctx.nodearray.items[0].next_by_letter, 'f')];
		const rl_node& node_fa = ctx.nodearray.items[rl_edgemap_find(node_f.next_by_letter, 'a')];
		const rl_node& node_fac = ctx.nodearray.items[rl_edgemap_find(node_fa.next_by_letter, 'c')];
		const rl_node& node_face = ctx.nodearray.items[rl_edgemap_find(node_fac.next_by_letter, 'e')];
		const int32 node_index_facet = rl_edgemap_find(node_face.next_by_letter, 't');
		t_assert(node_index_facet == 3);
	}

	// Adding 'facts' should tack a new node onto our 'fact' branch, with an 's' edge
	// leading to it: this brings our node count to 10
	// [0] -c-> [1] -a-> [2] -t-> [3] -s-> [4]
	//  |                  \e
	//  |                   \
	//  f-> [5] -a-> [6] -c-> [7] -t-> [8] -s-> [9]
	ok = rl_dawg_ctx_add(ctx, t_word("facts"), 5); t_assert(ok);
	t_assert(ctx.nodearray.size == 10);

	// Once we're finished adding words to a DAWG, we have to explicitly finalize it so
	// that any newly-added branches can be minimized: in this case, 'facTS' can be
	// merged into the same 'TS' suffix branch used by 'caTS' and 'faceTS':
	// [0] -c-> [1] -a-> [2] -t-> [3] -s-> [4]
	//  |                  \e    /t
	//  |                   \   /
	//  f-> [5] -a-> [6] -c-> [7]
	t_assert(ctx.edge_stack_size == 5);
	t_assert(ctx.edge_stack[0].letter == 'f');
	t_assert(ctx.edge_stack[1].letter == 'a');
	t_assert(ctx.edge_stack[2].letter == 'c');
	t_assert(ctx.edge_stack[3].letter == 't');
	t_assert(ctx.edge_stack[4].letter == 's');
	rl_dawg_ctx_finalize(ctx);
	t_assert(ctx.edge_stack_size == 0);
	t_assert(ctx.nodearray.size == 8);
	const rl_node& node_f = ctx.nodearray.items[rl_edgemap_find(ctx.nodearray.items[0].next_by_letter, 'f')];
	const rl_node& node_fa = ctx.nodearray.items[rl_edgemap_find(node_f.next_by_letter, 'a')];
	const rl_node& node_fac = ctx.nodearray.items[rl_edgemap_find(node_fa.next_by_letter, 'c')];
	const int32 node_index_fact = rl_edgemap_find(node_fac.next_by_letter, 't');
	t_assert(node_index_fact == 3);

	rl_dawg_ctx_free(ctx);
	return nullptr;
}

const char* test_dawg_ctx_move_nodes()
{
	// Use an rl_dawg_ctx to build up a set of valid words
	rl_dawg_ctx ctx;
	rl_dawg_ctx_init(ctx);
	bool ok;
	ok = rl_dawg_ctx_add(ctx, t_word("cat"), 3); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("cats"), 4); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("facet"), 5); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("facets"), 6); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("fact"), 4); t_assert(ok);
	ok = rl_dawg_ctx_add(ctx, t_word("facts"), 5); t_assert(ok);
	rl_dawg_ctx_finalize(ctx);

	// The rl_dawg holds the final set of nodes used for searches at runtime: it should
	// initially have no memory allocated
	rl_dawg dawg;
	rl_dawg_init(dawg);
	t_assert(ctx.nodearray.capacity >= ctx.nodearray.size);
	t_assert(ctx.nodearray.size == 8);
	t_assert(ctx.nodearray.items);
	t_assert(dawg.nodearray.capacity == 0);
	t_assert(dawg.nodearray.size == 0);
	t_assert(!dawg.nodearray.items);

	// Once the DAWG-building process is complete, nodes are moved from the rl_dawg_ctx
	// to the rl_dawg
	rl_dawg_ctx_move_nodes(ctx, dawg);
	t_assert(ctx.nodearray.capacity == 0);
	t_assert(ctx.nodearray.size == 0);
	t_assert(!ctx.nodearray.items);
	t_assert(dawg.nodearray.capacity >= dawg.nodearray.size);
	t_assert(dawg.nodearray.size == 8);
	t_assert(dawg.nodearray.items);

	rl_dawg_ctx_free(ctx);
	rl_dawg_free(dawg);
	return nullptr;
}

const char* test_dawg_build()
{
	rl_dawg dawg;
	rl_dawg_init(dawg);

	// Use our helper function from rl_testing.h to write a wordlist to a temp file and
	// pass it to rl_dawg_build
	const char* words = "cat\ncats\nfacet\nfacets\nfact\nfacts\n";
	const int32 num_words = rl_test_dawg_build(dawg, words);
	t_assert(num_words == 6);
	t_assert(dawg.nodearray.size == 8);

	// Traverse the resulting DAWG
	const rl_node& root = dawg.nodearray.items[0];
	const rl_node& f = dawg.nodearray.items[rl_edgemap_find(root.next_by_letter, 'f')];
	const rl_node& fa = dawg.nodearray.items[rl_edgemap_find(f.next_by_letter, 'a')];
	const rl_node& fac = dawg.nodearray.items[rl_edgemap_find(fa.next_by_letter, 'c')];
	const rl_node& fact = dawg.nodearray.items[rl_edgemap_find(fac.next_by_letter, 't')];
	const rl_node& facts = dawg.nodearray.items[rl_edgemap_find(fact.next_by_letter, 's')];
	t_assert(!f.is_word);
	t_assert(!fa.is_word);
	t_assert(!fac.is_word);
	t_assert(fact.is_word);
	t_assert(facts.is_word);
	t_assert(facts.next_by_letter.size == 0);

	// Verify the expected weights for each letter in the distribution
	// (aaaaaacccccceeffffssstttttt)
	t_assert(dawg.distribution.weights['a' - 'a'] == 0.222222224f); // 6 of 27
	t_assert(dawg.distribution.weights['b' - 'a'] == 0.0f);
	t_assert(dawg.distribution.weights['c' - 'a'] == 0.222222224f); // 6 of 27
	t_assert(dawg.distribution.weights['d' - 'a'] == 0.0f);
	t_assert(dawg.distribution.weights['e' - 'a'] == 0.0740740746f); // 2 of 27
	t_assert(dawg.distribution.weights['f' - 'a'] == 0.148148149f); // 4 of 27
	t_assert(dawg.distribution.weights['s' - 'a'] == 0.111111112f); // 3 of 27
	t_assert(dawg.distribution.weights['t' - 'a'] == 0.222222224f); // 6 of 27

	// Verify that all weights in the distribution sum to 1.0
	float weight_sum = 0.0f;
	for (size_t i = 0; i < COUNT_OF(dawg.distribution.weights); i++)
	{
		weight_sum += dawg.distribution.weights[i];
	}
	t_assert(weight_sum > 0.999999f && weight_sum < 1.000001f);

	rl_dawg_free(dawg);
	return nullptr;
}
