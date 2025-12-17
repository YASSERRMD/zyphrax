#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/zyphrax_lz77.h" // for match struct
#include "../src/zyphrax_seq.c"

void test_simple_sequence() {
  uint8_t lit[] = "hello";
  zyphrax_sequence_t seq = {
      .literals = lit, .lit_len = 5, .match = {.offset = 12, .length = 8}
      // match "world..."
  };

  uint8_t buf[128];
  size_t n = zyphrax_encode_sequence(&seq, buf, 128);

  // Check Analysis:
  // lit_len=5 (<15), match_len=8 (so 8-4=4, <15)
  // Token = (5 << 4) | 4 = 0x54.
  // Bytes: [54] [h][e][l][l][o] [0C][00] (offset 12)
  // Total len: 1 + 5 + 2 = 8 bytes.

  assert(n == 8);
  assert(buf[0] == 0x54);
  assert(memcmp(buf + 1, "hello", 5) == 0);
  assert(buf[6] == 12);
  assert(buf[7] == 0);

  printf("Simple sequence test passed.\n");
}

void test_long_sequence() {
  // Test extended lengths
  // Lit len 20 (>=15)
  // Match len 25 (25-4=21 >= 15)
  uint8_t lit[20];
  memset(lit, 'x', 20);

  zyphrax_sequence_t seq = {
      .literals = lit, .lit_len = 20, .match = {.offset = 500, .length = 25}};

  uint8_t buf[128];
  size_t n = zyphrax_encode_sequence(&seq, buf, 128);

  // Analysis:
  // Token: (15 << 4) | 15 = 0xFF
  // Extra lit len: 20-15 = 5. (1 byte: 0x05)
  // Literals: 20 bytes.
  // Offset: 500 = 0x01F4 -> [F4][01]
  // Extra match len: 25-4-15 = 6. (1 byte: 0x06)

  // Total: 1 (token) + 1 (lit ext) + 20 (lit) + 2 (off) + 1 (match ext) = 25
  // bytes.

  assert(n == 25);
  assert(buf[0] == 0xFF);
  assert(buf[1] == 5);
  assert(memcmp(buf + 2, lit, 20) == 0);
  // Offset at 2+20 = 22
  assert(buf[22] == 0xF4);
  assert(buf[23] == 0x01);
  // Match ext at 24
  assert(buf[24] == 6);

  printf("Long sequence test passed.\n");
}

int main() {
  test_simple_sequence();
  test_long_sequence();
  return 0;
}
