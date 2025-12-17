#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Include the implementation file directly to test static functions
#include "../src/zyphrax.c"

void test_packing() {
  uint8_t buf[128];
  memset(buf, 0, sizeof(buf));

  zyphrax_params_t p = {.level = 5, .block_size = 65536, .checksum = 1};

  zyphrax_write_header_internal(buf, &p);

  // Verify Magic (u32 LE match)
  uint32_t magic = read_u32_le(buf);
  assert(magic == ZYPHRAX_MAGIC);

  // Verify Flags/Size layout
  // We expect: Word1 = (block_size[24]) | (flags[8] << 24)
  // Flags for level=5, checksum=1 => 5 | (1<<3) = 13 (0x0D)
  // Block size 65536 = 0x010000
  // Expected Word1: 0x010000 | 0x0D000000 = 0x0D010000
  uint32_t word1 = read_u32_le(buf + 4);
  assert(word1 == 0x0D010000);

  // Verify Roundtrip
  zyphrax_params_t p2;
  int res = zyphrax_read_header_internal(buf, &p2);
  assert(res == 0);
  assert(p2.level == 5);
  assert(p2.block_size == 65536);
  assert(p2.checksum == 1);

  printf("Packing test passed.\n");
}

void test_public_api() {
  uint8_t src[100];
  uint8_t dst[100];
  zyphrax_params_t p = {.level = 9, .block_size = 1024, .checksum = 0};

  // compress_bound check
  size_t bound = zyphrax_compress_bound(100);
  // 100 + 100/255 + 64 = 100 + 0 + 64 = 164
  assert(bound >= 112);

  size_t sz = zyphrax_compress(src, 100, dst, 100, &p);
  assert(sz == 12);

  uint32_t magic = read_u32_le(dst);
  assert(magic == ZYPHRAX_MAGIC);

  printf("Public API test passed.\n");
}

int main() {
  test_packing();
  test_public_api();
  printf("All tests passed!\n");
  return 0;
}
