cmake_minimum_required(VERSION 4.0)

# roselex is a library that provides grid-based word game functionality
project(roselex VERSION 0.1.0)
enable_testing()

# Create a library target
add_library(roselex)
target_include_directories(roselex PUBLIC include)
target_sources(roselex
    PUBLIC
        include/rl_util.h
        include/rl_types.h
        include/rl_edgemap.h
        include/rl_nodearray.h
        include/rl_nodelookup.h
        include/rl_node.h
        include/rl_dawg.h
        include/rl_distribution.h
        include/rl_bag.h
        include/rl_rack.h
        include/rl_board.h
        include/rl_move.h
        include/rl_search.h
        include/rl_preview.h
    PRIVATE
        src/rl_edgemap.cpp
        src/rl_nodearray.cpp
        src/rl_nodelookup.cpp
        src/rl_node.cpp
        src/rl_dawg.cpp
        src/rl_distribution.cpp
        src/rl_bag.cpp
        src/rl_rack.cpp
        src/rl_board.cpp
        src/rl_move.cpp
        src/rl_search.cpp
        src/rl_preview.cpp
)

# Build a tests executable
add_executable(tests
    tests/main.cpp
    tests/testing.h
    tests/rl_types_tests.h
    tests/rl_edgemap_tests.h
    tests/rl_nodearray_tests.h
    tests/rl_nodelookup_tests.h
    tests/rl_node_tests.h
    tests/rl_dawg_tests.h
    tests/rl_distribution_tests.h
    tests/rl_bag_tests.h
    tests/rl_testing.h
)
target_link_libraries(tests PRIVATE roselex)
add_test(NAME run_tests COMMAND tests)

# Add benchmarks target
add_executable(benchmarks
    benchmarks/main.cpp
)
target_link_libraries(benchmarks PRIVATE roselex)
