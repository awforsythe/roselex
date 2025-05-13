#!/usr/bin/env bash
set -e

BUILD_DIR='./build_bench'
rm -rf "$BUILD_DIR"
mkdir "$BUILD_DIR"

cmake -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo .
cmake --build "$BUILD_DIR"

echo ""
echo "words,tiles,moves,elapsed_load_ns,elapsed_moves_ns"
WORDLISTS='basic alpha'
NUM_EXECUTIONS_PER_WORDLIST='10'
for WORDLIST in $WORDLISTS; do
	for I in $(seq $NUM_EXECUTIONS_PER_WORDLIST); do
		"$BUILD_DIR/benchmarks" "./data/words_$WORDLIST.txt" --csv
	done
done
