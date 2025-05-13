#!/usr/bin/env bash
set -e

BUILD_DIR='./build_bench'
if [ ! -d "$BUILD_DIR" ]; then
  mkdir "$BUILD_DIR"
  cmake -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo .
fi
cmake --build "$BUILD_DIR"

echo ""
echo "wordlist,num_allocs,num_frees,bytes_allocated,load_min,load_max,load_median,load_mean,load_stddev,moves_min,moves_max,moves_median,moves_mean,moves_stddev"
WORDLISTS='basic'
NUM_EXECUTIONS_PER_WORDLIST='10'
for WORDLIST in $WORDLISTS; do
  # Run valgrind (memcheck), failing if any leaks/errors are detected, and capture the
  # memory usage stats from our benchmark program
  echo -n "$WORDLIST"
  ALLOC_STATS=$(
    valgrind \
      --tool=memcheck \
      --leak-check=full \
      --error-exitcode=1 \
      --exit-on-first-error=yes \
      "$BUILD_DIR/benchmarks" "./data/words_$WORDLIST.txt" 2>&1 | \
        grep 'total heap usage' | \
        sed -E 's/.*: ([0-9,]+) allocs, ([0-9,]+) frees, ([0-9,]+) bytes allocated/\1 \2 \3/' | \
        tr -d ','
  )
  NUM_ALLOCATIONS=$(echo "$ALLOC_STATS" | cut -d' ' -f1)
  NUM_FREES=$(echo "$ALLOC_STATS" | cut -d' ' -f2)
  BYTES_ALLOCATED=$(echo "$ALLOC_STATS" | cut -d' ' -f3)
  echo -n ",$NUM_ALLOCATIONS,$NUM_FREES,$BYTES_ALLOCATED"

  # Run our benchmark several times, capturing the elapsed time taken to load the word
  # list and to search for and make several moves on the board
  ELAPSED_NS_LOAD_PY_LITERALS=""
  ELAPSED_NS_MOVES_PY_LITERALS=""
  for I in $(seq $NUM_EXECUTIONS_PER_WORDLIST); do
    if [ "$I" -gt 1 ]; then
      ELAPSED_NS_LOAD_PY_LITERALS="$ELAPSED_NS_LOAD_PY_LITERALS,"
      ELAPSED_NS_MOVES_PY_LITERALS="$ELAPSED_NS_MOVES_PY_LITERALS,"
    fi
    BENCHMARK_OUTPUT=$("$BUILD_DIR/benchmarks" "./data/words_$WORDLIST.txt")
    ELAPSED_NS_LOAD=$(echo "$BENCHMARK_OUTPUT" | grep -F 'elapsed(load): ' | cut -d' ' -f2)
    ELAPSED_NS_MOVES=$(echo "$BENCHMARK_OUTPUT" | grep -F 'elapsed(moves): ' | cut -d' ' -f2)
    ELAPSED_NS_LOAD_PY_LITERALS="$ELAPSED_NS_LOAD_PY_LITERALS$ELAPSED_NS_LOAD"
    ELAPSED_NS_MOVES_PY_LITERALS="$ELAPSED_NS_MOVES_PY_LITERALS$ELAPSED_NS_MOVES"
  done

  LOAD_MIN=$(python3 -c "print(min([$ELAPSED_NS_LOAD_PY_LITERALS]))")
  LOAD_MAX=$(python3 -c "print(max([$ELAPSED_NS_LOAD_PY_LITERALS]))")
  LOAD_MEDIAN=$(python3 -c "import statistics; print(int(statistics.median([$ELAPSED_NS_LOAD_PY_LITERALS])))")
  LOAD_MEAN=$(python3 -c "import statistics; print(int(statistics.mean([$ELAPSED_NS_LOAD_PY_LITERALS])))")
  LOAD_STDEV=$(python3 -c "import statistics; print(int(statistics.stdev([$ELAPSED_NS_LOAD_PY_LITERALS])))")
  echo -n "$LOAD_MIN,$LOAD_MAX,$LOAD_MEDIAN,$LOAD_MEAN,$LOAD_STDEV"

  MOVES_MIN=$(python3 -c "print(min([$ELAPSED_NS_MOVES_PY_LITERALS]))")
  MOVES_MAX=$(python3 -c "print(max([$ELAPSED_NS_MOVES_PY_LITERALS]))")
  MOVES_MEDIAN=$(python3 -c "import statistics; print(int(statistics.median([$ELAPSED_NS_MOVES_PY_LITERALS])))")
  MOVES_MEAN=$(python3 -c "import statistics; print(int(statistics.mean([$ELAPSED_NS_MOVES_PY_LITERALS])))")
  MOVES_STDEV=$(python3 -c "import statistics; print(int(statistics.stdev([$ELAPSED_NS_MOVES_PY_LITERALS])))")
  echo -n ",$MOVES_MIN,$MOVES_MAX,$MOVES_MEDIAN,$MOVES_MEAN,$MOVES_STDEV"

  echo ""
done
