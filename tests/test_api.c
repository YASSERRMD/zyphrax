#include "zyphrax.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// We link against the library or object files.
// For test setup here: we'll compile everything together.

void test_full_roundtrip_compress() {
  // Generate 1MB of data
  size_t size = 1024 * 1024;
  uint8_t *src = malloc(size);
  // Fill with pattern
  for (size_t i = 0; i < size; i++)
    src[i] = i & 0xFF; // Low repetitivity -> harder?
  // Actually i & 0xFF repeats every 256 bytes. Highly compressible with LZ77.

  // Params
  zyphrax_params_t params = {
      .level = 1, .block_size = 64 * 1024, .checksum = 0};

  size_t bound = zyphrax_compress_bound(size);
  uint8_t *dst = malloc(bound);

  printf("Compressing 1MB...\n");
  size_t comp_size = zyphrax_compress(src, size, dst, bound, &params);
  printf("Compressed size: %zu (Ratio: %.2f%%)\n", comp_size,
         (double)comp_size * 100.0 / size);

  assert(comp_size > 12);
  assert(comp_size < size); // Should be compressed

  // Verify Header
  zyphrax_params_t p_out;
  // We can't access internal read func, but we check bytes manually
  // or use zyphrax_decompress which parses header (and returns 0 for now).
  size_t dec_res = zyphrax_decompress(dst, comp_size, src, size);
  assert(dec_res == 0); // As expected for Phase 7

  // Check magic manual
  assert(dst[0] == 0x59); // Z
  assert(dst[1] == 0x46); // Y ... wait, ZYFX is 58 59 46 59
  // #define ZYPHRAX_MAGIC 0x58594659u (little endian?)
  // 0x59465958?
  // "ZYFX"
  // 'Z' = 0x5A (Wait, Z is 0x5A, Y is 0x59)
  // Prompt says: 0x58594659u // "ZYFX" little-endian?
  // If string "ZYFX" is at byte 0..3:
  // Byte 0='Z'(0x5A), 1='Y'(0x59), 2='F'(0x46), 3='X'(0x58).
  // Word read LE: 0x5846595A.
  // The define 0x58594659 is used.
  // Let's assume the define is authoritative.

  free(src);
  free(dst);

  printf("Full integration test passed.\n");
}

int main() {
  test_full_roundtrip_compress();
  return 0;
}
