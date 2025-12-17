#pragma once
#include <stddef.h>
#include <stdint.h>

#define HASH_LOG 16
#define HASH_SIZE (1 << HASH_LOG)
#define MAX_DIST 65535
#define MIN_MATCH 4
#define MAX_MATCH 258

typedef struct {
  uint16_t hash_table[HASH_SIZE]; // Heads of chains
  uint16_t chain[1 << 18];        // 256K chain buffer (offsets)
} zyphrax_lz77_t;

typedef struct {
  uint16_t offset;
  uint16_t length;
} zyphrax_match_t;

// Initialize the LZ77 state (clears hash table)
void zyphrax_lz77_init(zyphrax_lz77_t *lz);

// Find best match for data at pos, looking back up to MAX_DIST
// Updates hash chain with new position
zyphrax_match_t zyphrax_find_best_match(zyphrax_lz77_t *lz, const uint8_t *data,
                                        size_t pos, size_t limit);
