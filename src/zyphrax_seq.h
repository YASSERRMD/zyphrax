#pragma once
#include "zyphrax_lz77.h"
#include <stddef.h>
#include <stdint.h>

// Token Encoding Layout (compatible with LZ4)
// Token Byte: [lit_len:4][match_len:4]
// match_len in token is (actual_match_len - 4).
// If lit_len/match_len is 15 (0xF), extra bytes follow.
// Offset: u16 little-endian.

typedef struct {
  const uint8_t *literals;
  size_t lit_len;
  zyphrax_match_t match; // offset, length
} zyphrax_sequence_t;

// Encodes a sequence into destination buffer.
// Returns bytes written.
// Note: This does NOT check output bounds thoroughly for performance, caller
// must ensure dst_cap is sufficient. (In a robust implementation, we check.
// Here we should check.)
size_t zyphrax_encode_sequence(const zyphrax_sequence_t *seq, uint8_t *out,
                               size_t out_cap);

// Estimates worst-case size for a sequence
size_t zyphrax_seq_bound(const zyphrax_sequence_t *seq);
