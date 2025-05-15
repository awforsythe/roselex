#!/usr/bin/env bash
set -e

# Build a profile-optimized version of our benchmark executable
BUILD_DIR='./build_bench'
if [ ! -d "$BUILD_DIR" ]; then
  mkdir "$BUILD_DIR"
  cmake -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo .
fi
cmake --build "$BUILD_DIR" >/dev/null 2>&1

# Run valgrind/memcheck to profile allocation stats for building word lists
profile_allocation() {
  WORDLIST="$1"
  ALLOC_STATS=$(
    valgrind \
      --tool=memcheck \
      --leak-check=full \
      --error-exitcode=1 \
      --exit-on-first-error=yes \
      "$BUILD_DIR/benchmarks" "$WORDLIST" \
        --seed=12345 \
        --board-size-x=200 \
        --board-size-y=180 \
        --num-tiles=50 \
        --num-moves=1 \
        --num-searches=0 \
        --first-move-x=4 \
        --first-move-y=4 \
        --first-move-word=yesterday \
      2>&1 | \
        grep 'total heap usage' | \
        sed -E 's/.*: ([0-9,]+) allocs, ([0-9,]+) frees, ([0-9,]+) bytes allocated/\1 \2 \3/' | \
        tr -d ','
  )
  NUM_ALLOCATIONS=$(echo "$ALLOC_STATS" | cut -d' ' -f1)
  BYTES_ALLOCATED=$(echo "$ALLOC_STATS" | cut -d' ' -f3)
  echo "$NUM_ALLOCATIONS,$BYTES_ALLOCATED"
}

profile_peak_heap_usage() {
  rm -f ./massif.out
  WORDLIST="$1"
  valgrind \
    --tool=massif \
    --massif-out-file=./massif.out \
    "$BUILD_DIR/benchmarks" "$WORDLIST" \
      --seed=12345 \
      --board-size-x=200 \
      --board-size-y=180 \
      --num-tiles=50 \
      --num-moves=1 \
      --num-searches=0 \
      --first-move-x=4 \
      --first-move-y=4 \
      --first-move-word=yesterday \
  >/dev/null 2>&1
  PEAK_HEAP_BYTES=$(cat ./massif.out | grep mem_heap_B= | cut -d'=' -f2 | sort -n | tail -1)
  rm -f massif.out
  echo "$PEAK_HEAP_BYTES"
}

# Profile the time it takes to perform word list building and move solving
analyze_execution_stats() {
  PY_LITERALS="$1"
  MIN=$(python3 -c "print(min([$PY_LITERALS]))")
  MAX=$(python3 -c "print(max([$PY_LITERALS]))")
  MEDIAN=$(python3 -c "import statistics; print(int(statistics.median([$PY_LITERALS])))")
  MEAN=$(python3 -c "import statistics; print(int(statistics.mean([$PY_LITERALS])))")
  STDEV=$(python3 -c "import statistics; print(int(statistics.stdev([$PY_LITERALS])))")
  echo "$MIN,$MAX,$MEDIAN,$MEAN,$STDEV"
}

profile_execution_alpha() {
  LOAD_LITERALS=""
  MOVES_LITERALS=""
  for I in {1..10}; do
    BENCHMARK_OUTPUT=$(
      "$BUILD_DIR/benchmarks" ./data/words_alpha.txt \
        --seed=12345 \
        --board-size-x=200 \
        --board-size-y=180 \
        --num-tiles=50 \
        --num-moves=100 \
        --num-searches=0 \
        --first-move-x=4 \
        --first-move-y=4 \
        --first-move-word=yesterday
    )
    if [ "$I" -gt 1 ]; then
      LOAD_LITERALS="$LOAD_LITERALS,"
      MOVES_LITERALS="$MOVES_LITERALS,"
    fi
    ELAPSED_NS_LOAD=$(echo "$BENCHMARK_OUTPUT" | grep -F 'elapsed(load): ' | cut -d' ' -f2)
    ELAPSED_NS_MOVES=$(echo "$BENCHMARK_OUTPUT" | grep -F 'elapsed(moves): ' | cut -d' ' -f2)
    LOAD_LITERALS="$LOAD_LITERALS$ELAPSED_NS_LOAD"
    MOVES_LITERALS="$MOVES_LITERALS$ELAPSED_NS_MOVES"
  done
  LOAD_STATS=$(analyze_execution_stats "$LOAD_LITERALS")
  MOVES_STATS=$(analyze_execution_stats "$MOVES_LITERALS")
  echo "$LOAD_STATS,$MOVES_STATS"
}

profile_execution_basic() {
  LOAD_LITERALS=""
  SEARCHES_LITERALS=""
  for I in {1..10}; do
    BENCHMARK_OUTPUT=$(
      "$BUILD_DIR/benchmarks" ./data/words_basic.txt \
        --seed=12345 \
        --board-size-x=200 \
        --board-size-y=180 \
        --num-tiles=50 \
        --num-moves=0 \
        --num-searches=200 \
        --first-move-x=4 \
        --first-move-y=4 \
        --first-move-word=yesterday
    )
    if [ "$I" -gt 1 ]; then
      LOAD_LITERALS="$LOAD_LITERALS,"
      SEARCHES_LITERALS="$SEARCHES_LITERALS,"
    fi
    ELAPSED_NS_LOAD=$(echo "$BENCHMARK_OUTPUT" | grep -F 'elapsed(load): ' | cut -d' ' -f2)
    ELAPSED_NS_SEARCHES=$(echo "$BENCHMARK_OUTPUT" | grep -F 'elapsed(searches): ' | cut -d' ' -f2)
    LOAD_LITERALS="$LOAD_LITERALS$ELAPSED_NS_LOAD"
    SEARCHES_LITERALS="$SEARCHES_LITERALS$ELAPSED_NS_SEARCHES"
  done
  LOAD_STATS=$(analyze_execution_stats "$LOAD_LITERALS")
  SEARCHES_STATS=$(analyze_execution_stats "$SEARCHES_LITERALS")
  echo "$LOAD_STATS,$SEARCHES_STATS"
}

echo -n 'basic_num_allocations,basic_bytes_allocated'
echo -n ',basic_peak_heap'
echo -n ',basic_load_min,basic_load_max,basic_load_median,basic_load_mean,basic_load_stdev'
echo -n ',basic_searches_min,basic_searches_max,basic_searches_median,basic_searches_mean,basic_searches_stdev'
echo -n ',alpha_num_allocations,alpha_bytes_allocated'
echo -n ',alpha_peak_heap'
echo -n ',alpha_load_min,alpha_load_max,alpha_load_median,alpha_load_mean,alpha_load_stdev'
echo -n ',alpha_moves_min,alpha_moves_max,alpha_moves_median,alpha_moves_mean,alpha_moves_stdev'
echo ''

BASIC_ALLOC_STATS=$(profile_allocation ./data/words_basic.txt)
echo -n "$BASIC_ALLOC_STATS"

BASIC_PEAK_HEAP=$(profile_peak_heap_usage ./data/words_basic.txt)
echo -n ",$BASIC_PEAK_HEAP"

EXECUTION_BASIC_STATS=$(profile_execution_basic)
echo -n ",$EXECUTION_BASIC_STATS"

ALPHA_ALLOC_STATS=$(profile_allocation ./data/words_alpha.txt)
echo -n ",$ALPHA_ALLOC_STATS"

ALPHA_PEAK_HEAP=$(profile_peak_heap_usage ./data/words_alpha.txt)
echo -n ",$ALPHA_PEAK_HEAP"

EXECUTION_ALPHA_STATS=$(profile_execution_alpha)
echo -n ",$EXECUTION_ALPHA_STATS"

echo ''
