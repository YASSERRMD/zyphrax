#include "zyphrax.h"
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

// Header size is fixed 12 bytes
#define HEADER_SIZE 12

// Flags packing:
// [reserved:3][raw_block:1][checksum:1][level:3]
// We'll put this in the high byte of the 2nd word?
// Structure:
// Bytes 0-3: Magic
// Bytes 4-6: Block Size (24 bits)
// Byte 7: Flags
// Bytes 8-11: CRC32 (Header Checksum or reserved for now?)

// Actually, let's follow standard bit packing if we put Flags inside
// appropriate bits But to keep it simple and aligned: Word 0: Magic Word 1:
// BlockSize (low 24) | (Flags << 24) Word 2: Checksum (Content checksum? Header
// checksum? Let's assume header checksum or 0) For Phase 1, we will just write
// 0 for CRC32 field as we don't have CRC impl yet.

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
  // Simple bound: src + src/something + 12.
  // For raw storage worst case: src + header + block_headers needed
  // If block size is 64KB, we have src_size / 64KB blocks.
  // Each block might need framing?
  // Let's safe-bound: src + 12 + (src/64 + 1)*16 (block overhead?)
  // LZ4 uses src + src/255 + 16.
  return src_size + (src_size / 255) + 64;
}

size_t zyphrax_compress(const uint8_t *src, size_t src_size, uint8_t *dst,
                        size_t dst_cap, const zyphrax_params_t *params) {
  if (dst_cap < 12)
    return 0;

  zyphrax_write_header_internal(dst, params);

  // For Phase 1, we don't compress. We just write header and stop.
  // Or should we copy raw?
  // "Deliverable: Header read/write functions complete."
  // It doesn't say "Valid compressor".
  // I will return 12 (bytes written).

  return 12;
}

size_t zyphrax_decompress(const uint8_t *src, size_t src_size, uint8_t *dst,
                          size_t dst_cap) {
  if (src_size < 12)
    return 0;

  zyphrax_params_t params;
  if (zyphrax_read_header_internal(src, &params) != 0) {
    return 0; // Error
  }

  // For Phase 1, nothing to decompress yet except verifying header.
  // Return 0 decompressed bytes?
  return 0;
}
