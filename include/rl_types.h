#pragma once

#include <cinttypes>
#include <cstddef>

typedef uint8_t uint8;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint64_t uint64;

// Maximum acceptable length for a word that may be added to a DAWG: if a word list
// contains words that exceed this length, those words will be rejected.
static const size_t RL_MAX_WORD_LEN = 32;
