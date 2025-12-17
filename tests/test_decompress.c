#include "zyphrax.h"
#include "zyphrax_dec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Include implementation for test linkage
#include "../src/zyphrax_dec.c"

void test_dec_table() {
  zyphrax_huff_decoder dec;
  uint8_t lens[256] = {0};

  // Simulating: A=1 bit, B=2 bits, C=2 bits?
  // Canonical:
  // Len 1: 1 code. Code 0.
  // Len 2: 2 codes. Code 10, 11 (binary 2, 3)
  // Map: A->len 1, B->len 2, C->len 2.
  lens['A'] = 1;
  lens['B'] = 2;
  lens['C'] = 2;

  zyphrax_build_dec_table(&dec, lens);

  // Verify Table
  // A (len 1, code 0) -> reversed 0.
  // Entries: 0, 2, 4, ... (even indices) should match A.
  // Entry format: [sym:8][len:8].
  // A is 65 (0x41).
  // Expected: (0x41 << 8) | 1 = 0x4101.

  // B (len 2, code 2=10) -> reversed 01 (1).
  // Entries 1, 5, 9... (mod 4 = 1)

  // C (len 2, code 3=11) -> reversed 11 (3).
  // Entries 3, 7, 11... (mod 4 = 3)

  // Check some indices
  assert(dec.table[0] == 0x4101);           // 00... -> A
  assert(dec.table[1] == (('B' << 8) | 2)); // 01... -> B
  assert(dec.table[2] == 0x4101);           // 10... -> A
  assert(dec.table[3] == (('C' << 8) | 2)); // 11... -> C

  printf("Decoder table test passed.\n");
}

int main() {
  test_dec_table();
  return 0;
}
