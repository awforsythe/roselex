#include <cstdio>

#include "testing.h"
#include "rl_types_tests.h"
#include "rl_edgemap_tests.h"
#include "rl_node_tests.h"
#include "rl_nodearray_tests.h"
#include "rl_nodelookup_tests.h"
#include "rl_dawg_tests.h"
#include "rl_distribution_tests.h"
#include "rl_rack_tests.h"
#include "rl_bag_tests.h"

/*
	Runs all tests in the roselexlib library. This is a good entry point for
	understanding how the roselexlib codebase is structured, what it does, and what
	each data type or set of functions is responsible for.

	As new tests are added, they need to be explicitly invoked in a t_run macro. We
	don't have any fancy test-runner tools to gather all test cases procedurally: this
	is just a plain C++ program using some macros defined in testing.h.
*/
int main(int argc, char* argv[])
{
	t_begin();

	// rl_types.h defines basic numeric types and constants for use by this library
	t_run(test_types_sizes);
	t_run(test_types_constants);

	// rl_edgemap is a basic data structure that lets us keep track of the edges between
	// nodes in the DAWG
	t_run(test_edgemap_init);
	t_run(test_edgemap_insert);
	t_run(test_edgemap_find);
	t_run(test_edgemap_replace);

	// rl_node represents a single node in the DAWG, with each containing its own
	// edgemap
	t_run(test_node_init);
	t_run(test_node_reset);
	t_run(test_node_signature);

	// rl_nodearray stores all the nodes in the DAWG in a flat array
	t_run(test_nodearray_init);
	t_run(test_nodearray_push);
	t_run(test_nodearray_pop);

	// rl_nodelookup is a hash map that allows us to register the association between
	// an rl_node's unique signature (i.e. hash) and its index in the nodearray,
	// facilitating fast lookups when building and finalizing the DAWG
	t_run(test_nodelookup_init);
	t_run(test_nodelookup_insert);
	t_run(test_nodelookup_find);

	// rl_dawg represents a "directed acyclic word graph", or DAWG. The DAWG is built
	// from a (potentially very large) list of legal words - it's a space-efficient
	// representation of those words that can be efficiently traversed to find legal
	// moves. rl_dawg_ctx is a helper struct used during the building of the DAWG.
	t_run(test_dawg_ctx_init);
	t_run(test_dawg_ctx_add);
	t_run(test_dawg_ctx_finalize);
	t_run(test_dawg_ctx_move_nodes);
	t_run(test_dawg_build);

	// rl_distribution is a set of weights recording how prevalent any given letter is
	// within a set of letters (e.g. words in an input word list, letter tiles in a rack)
	t_run(test_distribution_init);
	t_run(test_distribution_from_rack);
	t_run(test_distribution_compare);
	t_run(test_distribution_get_random_letter);
	t_run(test_distribution_get_best_letters);

	// rl_rack is a finite set of letter tiles, 0 or more for each letter, that can be
	// used to play valid moves to the board
	t_run(test_rack_init);
	t_run(test_rack_push);
	t_run(test_rack_find);
	t_run(test_rack_pop);
	t_run(test_rack_subtract);

	t_run(test_bag_init);
	t_run(test_bag_draw);

	t_end();

	return 0;
}
