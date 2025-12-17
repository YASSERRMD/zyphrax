#include "zyphrax.h"
#include "zyphrax_block.h"
#include <string.h>

// Internal helper to write 32-bit LE
static void write_u32_le(uint8_t *p, uint32_t x) {
  p[0] = x & 0xFF;
  p[1] = (x >> 8) & 0xFF;
  p[2] = (x >> 16) & 0xFF;
  p[3] = (x >> 24) & 0xFF;
}

// Internal helper to read 32-bit LE
static uint32_t read_u32_le(const uint8_t *p) {
  return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

// -------------------------------------------------------------------------
// Header Implementation
// -------------------------------------------------------------------------

#define HEADER_SIZE 12

static void build_flags(const zyphrax_params_t *params, uint8_t *flags_out) {
  uint8_t level = (params->level & 0x7);            // 3 bits
  uint8_t checksum = (params->checksum & 0x1) << 3; // 1 bit
  uint8_t raw = 0; // Not raw block in frame header usually
  uint8_t reserved = 0;

  *flags_out = level | checksum | (raw << 4) | (reserved << 5);
}

static void parse_flags(uint8_t flags_in, zyphrax_params_t *params) {
  params->level = flags_in & 0x7;
  params->checksum = (flags_in >> 3) & 0x1;
  // raw and reserved ignored for params struct
}

void zyphrax_write_header_internal(uint8_t *dst,
                                   const zyphrax_params_t *params) {
  write_u32_le(dst, ZYPHRAX_MAGIC);

  uint8_t flags;
  build_flags(params, &flags);

  uint32_t block_size = params->block_size;
  if (block_size > 0xFFFFFF) {
    block_size = 0xFFFFFF; // Clamp to 24-bit max (16MB)
  }

  uint32_t word1 = (block_size & 0xFFFFFF) | ((uint32_t)flags << 24);
  write_u32_le(dst + 4, word1);

  // Checksum: Placeholder 0 for now (or TODO: Implement CRC)
  write_u32_le(dst + 8, 0x00000000);
}

int zyphrax_read_header_internal(const uint8_t *src, zyphrax_params_t *params) {
  uint32_t magic = read_u32_le(src);
  if (magic != ZYPHRAX_MAGIC)
    return -1; // Invalid magic

  uint32_t word1 = read_u32_le(src + 4);
  uint32_t block_size = word1 & 0xFFFFFF;
  uint8_t flags = (word1 >> 24) & 0xFF;

  parse_flags(flags, params);
  params->block_size = block_size;

  // Checksum validation skipped for now

  return 0;
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

size_t zyphrax_compress_bound(size_t src_size) {
  // Header (12) + src_size + worst case overhead
  // For raw storage worst case: src + header + block_headers (1 byte/block?)
  // If block size is 64KB, we have src_size / 64KB blocks.
  // Each block has 1 byte overhead for raw flag, plus potentially internal
  // overhead logic. Huffman overhead is ~384 bytes per block if compressed. But
  // we fallback to raw if expansion. Raw fallback is: src + 1. So per block
  // worst case is src_block + 1. Num blocks = (src + 65535) / 65536. Total
  // overhead = 12 + NumBlocks. Just to be safe: + 64 bytes base + src/255.
  return src_size + (src_size / 255) + 256;
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

size_t zyphrax_compress(const uint8_t *src, size_t src_size, uint8_t *dst,
                        size_t dst_cap, const zyphrax_params_t *params) {
  if (dst_cap < 12)
    return 0;

  // Use defaults if params is NULL? Or assume valid.
  // Let's copy to local to handle defaults
  zyphrax_params_t p = *params;
  if (p.block_size == 0)
    p.block_size = ZYPHRAX_BLOCK_SIZE;

  uint8_t *out = dst;
  uint8_t *out_end = dst + dst_cap;

  zyphrax_write_header_internal(out, &p);
  out += 12;

  size_t pos = 0;
  while (pos < src_size) {
    size_t block_size = min(p.block_size, src_size - pos);
    size_t rem_cap = out_end - out;

    size_t block_enc =
        zyphrax_compress_block(src + pos, block_size, out, rem_cap, &p);

    if (block_enc == 0)
      return 0; // Error / overflow

    out += block_enc;
    pos += block_size;
  }

  return out - dst;
}

size_t zyphrax_decompress(const uint8_t *src, size_t src_size, uint8_t *dst,
                          size_t dst_cap) {
  // Phase 9: Decompression
  // For now we just parse header
  if (src_size < 12)
    return 0;

  zyphrax_params_t params;
  if (zyphrax_read_header_internal(src, &params) != 0) {
    return 0; // Error
  }

  // Decompression logic pending (Phase 9)
  return 0;
}
