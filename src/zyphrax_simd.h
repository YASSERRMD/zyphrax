#pragma once
#include <stddef.h>
#include <stdint.h>

// Returns length of match between a and b, up to max_len
// Uses SIMD (AVX2 on x86, NEON on ARM) if available
size_t zyphrax_match_len_simd(const uint8_t *a, const uint8_t *b,
                              size_t max_len);

// Batch matching prototype (Phase 3 requirement)
// Process multiple positions in parallel
#include "zyphrax_lz77.h" // Needed for typedef if we use it directly, or use struct forward decl

// We will use struct pointer to avoid circular deps if header is standalone?
// But lz77_h is included.
void zyphrax_batch_match_simd(zyphrax_lz77_t *lz, const uint8_t *data,
                              size_t *positions, size_t count,
                              zyphrax_match_t *matches);
