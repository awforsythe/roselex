#pragma once

#include "rl_types.h"
#include "rl_nodearray.h"
#include "rl_nodelookup.h"
#include "rl_distribution.h"

/*
	Record of an edge that's been recently appended to the rl_dawg_ctx's nodearray, but
	that hasn't yet been considered for minimization. DAWG minimization is the process of
	merging newly-added branches back into existing nodes in cases where words with
	different prefixes share the same set of suffixes.
*/
struct rl_pending_edge
{
	// Index of the node from which this edge leads
	int32 from_index;

	// Index of the newly-added node at the end of this edge
	int32 to_index;

	// Letter labeling this edge
	uint8 letter;
};

/*
	Data structure used when building the DAWG from a word list. Words are added from
	an input word list one at a time, sorted in lexicographical order. As each word is
	added, new nodes are added into the nodearray, and new edges are established
	between those nodes, each labeled with a letter. Throughout the process, the DAWG
	is minimized, merging identical branches to keep the graph compact. Once all words
	are added, the DAWG is finalized, and the final array of nodes is moved to an
	rl_dawg struct.
*/
struct rl_dawg_ctx
{
	// Array of nodes under construction: as words are added to the DAWG, the graph is
	// modified by appending new nodes and converging identical branches.
	rl_nodearray nodearray;

	// Mapping of existing nodes by their unique signature, allowing nodes to be
	// deduplicated during minimization
	rl_nodelookup minimized_lookup;

	// Stack of newly-appended edges that have not yet been considered for minimization
	rl_pending_edge edge_stack[RL_MAX_WORD_LEN];
	int32 edge_stack_size;

	// The last word added to the DAWG; used to ensure that words are added in strict
	// lexicographical order
	uint8 prev_word[RL_MAX_WORD_LEN];
	int32 prev_word_len;

	// Sum of each letter encountered in the input word list; used for computing the
	// distribution of each letter in the final DAWG - prefix/suffix overlaps are
	// irrelevant; e.g. 'cat' and 'cats' represents 7 total letters, not 4
	uint32 letter_counts[26];
	uint32 letter_counts_sum;
};

/*
	Directed Acyclic Word Graph, or DAWG, as described by Appel & Jacobson in
	Communications of the ACM Vol 31 No 5, May 1988:
	https://www.cs.cmu.edu/afs/cs/academic/class/15451-s06/www/lectures/scrabble.pdf

	See also: rl_node.h.
*/
struct rl_dawg
{
	// Final, fully-minimized array of nodes, with items[0] being the root of the DAWG.
	// Valid words can be found by traversing the DAWG letter-by-letter, starting at
	// the root and following the edge labeled with the desired letter at each
	// iteration.
	rl_nodearray nodearray;

	// Final weights representing how common each letter is in the input word list;
	// summing to 1.0
	rl_distribution distribution;
};

// Initializes a new rl_dawg_ctx. You must call rl_daw_ctx_free when done. These
// functions are used internally by rl_dawg_build: you generally shouldn't need to call
// them directly.
void rl_dawg_ctx_init(rl_dawg_ctx& ctx);

// Releases all memory currently owned by the context.
void rl_dawg_ctx_free(rl_dawg_ctx& ctx);

// Adds a new word to the DAWG. Valid words consist only of lowercase ASCII letters 'a'
// through 'z' and must not exceed RL_MAX_WORD_LEN. Words must be added in alphabetical
// order. Returns true if the word was accepted and added to the DAWG; false if the
// word was rejected as invalid.
bool rl_dawg_ctx_add(rl_dawg_ctx& ctx, const uint8* word, int32 word_len);

// Finalizes the DAWG, performing a final minimization pass to ensure that all branches
// are merged.
void rl_dawg_ctx_finalize(rl_dawg_ctx& ctx);

// Transfers ownership of the context's nodearray to the target rl_dawg.
void rl_dawg_ctx_move_nodes(rl_dawg_ctx& ctx, rl_dawg& dawg);

// Initializes a new DAWG, with no initial memory allocation: the rl_dawg takes
// ownership of an existing nodearray from an rl_dawg_ctx. You must call rl_dawg_free
// when finished.
void rl_dawg_init(rl_dawg& dawg);

// Releases all memory currently owned by the DAWG.
void rl_dawg_free(rl_dawg& dawg);

// Reads a list of alphabetically-sorted words from the given ASCII text file, building
// a DAWG from that set of words. Uses rl_dawg_ctx internally. The input dawg must
// already be initialized. Returns the total number of words that were accepted and
// added to the DAWG.
int32 rl_dawg_build(rl_dawg& dawg, const char* wordlist_path);
