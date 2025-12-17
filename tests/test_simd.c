#include "../src/zyphrax_simd.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_simd_match() {
  uint8_t a[100];
  uint8_t b[100];

  // Fill with pattern
  for (int i = 0; i < 100; i++) {
    a[i] = i;
    b[i] = i;
  }

  // Test exact match
  size_t len = zyphrax_match_len_simd(a, b, 100);
  assert(len == 100);

  // Test mismatch
  b[50] = 0xFF; // Mismatch at 50
  len = zyphrax_match_len_simd(a, b, 100);
  assert(len == 50);

  // Test mismatch at start
  b[0] = 0xFF;
  len = zyphrax_match_len_simd(a, b, 100);
  assert(len == 0);

  printf("SIMD match length test passed.\n");
}

int main() {
  test_simd_match();
  return 0;
}
