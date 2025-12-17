#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/zyphrax_lz77.c"

void test_basic_match() {
  zyphrax_lz77_t lz;
  zyphrax_lz77_init(&lz);

  // "abcde" ... "abcde"
  // 01234       56789
  // Match should be found at pos 5, length 5, offset 5.
  uint8_t data[] = "abcdeabcde";
  size_t len = 10;

  // Insert first 5 bytes into hash (simulate sliding window)
  for (size_t i = 0; i < 5; i++) {
    zyphrax_find_best_match(&lz, data, i, len);
  }

  // Now at pos 5, we expect a match
  zyphrax_match_t m = zyphrax_find_best_match(&lz, data, 5, len);

  printf("Match: off=%d len=%d\n", m.offset, m.length);
  assert(m.length == 5);
  assert(m.offset == 5);

  printf("Basic match test passed.\n");
}

void test_random_data() {
  // Should verify non-crashing on random data
  zyphrax_lz77_t lz;
  zyphrax_lz77_init(&lz);

  uint8_t data[1000];
  for (int i = 0; i < 1000; i++)
    data[i] = rand() % 256;

  for (size_t i = 0; i < 900; i++) {
    zyphrax_find_best_match(&lz, data, i, 1000);
  }
  printf("Random data test passed.\n");
}

void benchmark_lz77() {
  // Generate 10MB of data with some redundancy
  size_t size = 10 * 1024 * 1024;
  uint8_t *buffer = malloc(size);
  for (size_t i = 0; i < size; i++) {
    buffer[i] = (i % 100) + (i % 3); // highly repetitive
  }

  zyphrax_lz77_t *lz = malloc(sizeof(zyphrax_lz77_t));
  zyphrax_lz77_init(lz);

  clock_t start = clock();

  // Process full buffer simulating greedy parsing
  size_t i = 0;
  while (i < size - MAX_MATCH) {
    zyphrax_match_t m = zyphrax_find_best_match(lz, buffer, i, size);
    if (m.length >= MIN_MATCH) {
      i += m.length;
    } else {
      i++;
    }
  }

  clock_t end = clock();
  double secs = (double)(end - start) / CLOCKS_PER_SEC;
  double mb_s = (size / (1024.0 * 1024.0)) / secs;

  printf("Processed 10MB in %.3fs = %.2f MB/s\n", secs, mb_s);

  free(lz);
  free(buffer);
}

int main() {
  test_basic_match();
  test_random_data();
  benchmark_lz77();
  return 0;
}
