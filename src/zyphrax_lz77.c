#include "zyphrax_lz77.h"
#include "zyphrax_simd.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline uint16_t zyphrax_hash4(const uint8_t *p) {
  uint32_t v = ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
  return (v * 2654435761u) >> (32 - HASH_LOG);
}

void zyphrax_lz77_init(zyphrax_lz77_t *lz) {
  memset(lz->hash_table, 0, sizeof(lz->hash_table));
  memset(lz->chain, 0, sizeof(lz->chain));
}

// Find best match
// Note: pos is absolute position in 'data'. 'limit' is the end of valid data.
zyphrax_match_t zyphrax_find_best_match(zyphrax_lz77_t *lz, const uint8_t *data,
                                        size_t pos, size_t limit) {
  zyphrax_match_t best_match = {0, 0};

  // Need at least MIN_MATCH bytes remaining
  if (pos + MIN_MATCH > limit) {
    return best_match;
  }

  uint16_t h = zyphrax_hash4(data + pos);

  // Use pos+1 so that 0 can be the sentinel for "empty"
  // When we retrieve, we get (old_pos + 1).
  // Delta = (current_pos + 1) - (old_pos + 1) = current_pos - old_pos.
  // This preserves delta calculation.

  // Note: If pos+1 wraps to 0 (at 65535), we effectively store 0 (empty).
  // This creates a blind spot at pos=65535, 131071, etc. Acceptable.

  uint16_t cur_val = lz->hash_table[h];

  // Update chain and hash table
  size_t chain_mask = (1 << 18) - 1;
  lz->chain[pos & chain_mask] = cur_val;
  lz->hash_table[h] = (uint16_t)(pos + 1);

  // Scan chain
  uint32_t max_chain_len = 256; // Limit search for speed

  uint16_t best_len = MIN_MATCH - 1;
  // Limit max match check
  size_t max_possible_match = MAX_MATCH;
  if (pos + max_possible_match > limit)
    max_possible_match = limit - pos;

  size_t depth = 0;

  // Current biased position for delta calc
  uint16_t scan_val = (uint16_t)(pos + 1);

  while (cur_val != 0 && depth++ < max_chain_len) {
    // Calculate distance
    // Since both scan_val and cur_val are biased by +1, difference is true
    // distance
    uint16_t delta = scan_val - cur_val;

    // Wrap-around distance check
    if (delta == 0 || delta > MAX_DIST) {
      // Invalid distance
    }

    // Safety: verify absolute position
    if (delta > pos) {
      break;
    }

    size_t match_full_pos = pos - delta;
    const uint8_t *candidate = data + match_full_pos;

    // Check match
    if (data[pos + best_len] == candidate[best_len] &&
        data[pos] == candidate[0]) {

      size_t len =
          zyphrax_match_len_simd(data + pos, candidate, max_possible_match);

      if (len > best_len) {
        best_len = len;
        best_match.offset = delta;
        best_match.length = len;

        if (len >= max_possible_match)
          break;
      }
    }

    // Next in chain
    cur_val = lz->chain[match_full_pos & chain_mask];
  }

  return best_match;
}
