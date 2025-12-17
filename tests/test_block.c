#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/zyphrax_block.c"
// We need to link everything. Tests usually just include the unit under test's
// dependencies. Since block.c includes the others via headers, we need to link
// the .c files or include them. But we included .c in previous phases. To avoid
// symbol collision if we include everything again: zyphrax_block.c include
// chain:
//   block.h
//   lz77.h
//   seq.h
//   huff.h
//   string, stdlib
// The implementation calls functions from lz77.c, seq.c, huff.c.
// We should compile those separate or include them here.
// Safest for unit test: Compile them together or include all .c?
// If I includes all .c, I might get duplicate static functions if not careful.
// But they have `zyphrax_` prefix public functions.
// Let's rely on gcc compilation linking.

void test_compress_small() {
  uint8_t src[] = "hello hello hello hello";
  size_t src_len = strlen((char *)src);
  uint8_t dst[1024];

  // Params ignored by block compressor currently
  zyphrax_params_t p = {0};

  size_t sz = zyphrax_compress_block(src, src_len, dst, 1024, &p);

  printf("Compressed 'hello...' (%zu) -> %zu bytes\n", src_len, sz);

  // Expect compression:
  // "hello " (6) literal
  // "hello " (6) match
  // "hello " (6) match
  // "hello" (5) match?
  // Should be smaller than src_len
  // But header overhead (trees) is 384 bytes?
  // Wait, my huffman encoder writes 3 * 128 bytes = 384 bytes header!
  // For small string, this explodes size.
  // So it should fallback to RAW (sz = src_len + 1).

  assert(sz == src_len + 1);
  assert(dst[0] == 0); // Raw type
  assert(memcmp(dst + 1, src, src_len) == 0);

  printf("Small block fallback test passed.\n");
}

void test_compress_large() {
  // 100KB of redundant data to ensure compression > overhead
  size_t len = 100 * 1024;
  uint8_t *src = malloc(len);
  for (size_t i = 0; i < len; i++)
    src[i] = 'A'; // Highly compressible

  uint8_t *dst = malloc(len * 2);

  clock_t start = clock();
  size_t sz = zyphrax_compress_block(src, len, dst, len * 2, NULL);
  clock_t end = clock();

  double secs = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = len / (1024.0 * 1024.0);
  printf("Compressed 100KB 'A's -> %zu bytes in %.3fs (%.2f MB/s)\n", sz, secs,
         mb / secs);

  // Overhead: ~384 bytes.
  // Compressed data: 1 sequence?
  // Lit 'A' (1 byte), Match 'A' * (len-1) -> Match Ref?
  // Block size is 100KB.
  // LZ77 window is 64KB.
  // Match will cap at 258.
  // So we will have many sequences of (Len 258, offset 1).
  // Each takes 1 token + 0 lit + 2 offset + 1 ext match len.
  // Total encoded size should be small.
  // 100KB / 258 * 4 bytes ~= 1.5KB.
  // Total < 2KB + 384 bytes.

  assert(sz < len);
  assert(sz > 0);
  assert(dst[0] == 1); // Compressed

  free(src);
  free(dst);

  printf("Large block compression test passed.\n");
}

int main() {
  test_compress_small();
  test_compress_large();
  return 0;
}
