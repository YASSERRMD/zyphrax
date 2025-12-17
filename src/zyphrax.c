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

#include "zyphrax_dec.h"

// Decompression Helper: Read Bits
typedef struct {
  const uint8_t *ptr;
  const uint8_t *end;
  uint64_t bit_buf;
  int bit_count;
} z_bit_reader;

static inline void refill_bits(z_bit_reader *br) {
  while (br->bit_count <= 56 && br->ptr < br->end) {
    br->bit_buf |= ((uint64_t)(*br->ptr++)) << br->bit_count;
    br->bit_count += 8;
  }
}

static inline uint16_t peek_bits(z_bit_reader *br, int n) {
  return (uint16_t)(br->bit_buf & ((1 << n) - 1));
}

static inline void consume_bits(z_bit_reader *br, int n) {
  br->bit_buf >>= n;
  br->bit_count -= n;
}

static inline uint16_t read_bits(z_bit_reader *br, int n) {
  uint16_t val = peek_bits(br, n);
  consume_bits(br, n);
  return val;
}

// Decode symbol using table
static inline uint16_t decode_sym(z_bit_reader *br,
                                  const zyphrax_huff_decoder *dec) {
  refill_bits(br);
  // Peek 15 bits max table size
  uint16_t look = peek_bits(br, 15);
  uint16_t entry = dec->table[look];
  // Entry: [sym:8][bits:8]
  uint8_t bits = entry & 0xFF;
  uint8_t sym = entry >> 8;
  consume_bits(br, bits);
  return sym;
}

static size_t read_start_extra(z_bit_reader *br) {
  size_t val = 0;
  while (1) {
    refill_bits(br);
    uint8_t b = read_bits(br, 8);
    val += b;
    if (b < 255)
      break;
  }
  return val;
}

size_t zyphrax_decompress(const uint8_t *src, size_t src_size, uint8_t *dst,
                          size_t dst_cap) {
  if (src_size < 12)
    return 0;

  zyphrax_params_t params;
  if (zyphrax_read_header_internal(src, &params) != 0)
    return 0;

  const uint8_t *in = src + 12; // Skip header
  const uint8_t *in_end = src + src_size;
  uint8_t *out = dst;
  uint8_t *out_end = dst + dst_cap;

  while (in < in_end) {
    // Read Block Type
    if (in >= in_end)
      break;
    uint8_t type = *in++;

    if (type == 0) { // Raw
      // We don't store raw block size in header.
      // RAW format assumes we know size? Or "rest of stream"?
      // Block format flaw here: Raw mode uses "store_raw" which writes
      // [0][Bytes]. It doesn't write length! Assuming raw is ONLY used for
      // fallback of entire buffer. Or fixed block size. Default block size
      // check:
      size_t rem_in = in_end - in;
      size_t rem_out = out_end - out;
      size_t chunk = (rem_in < params.block_size) ? rem_in : params.block_size;
      if (chunk > rem_out)
        return 0; // Overflow
      memcpy(out, in, chunk);
      in += chunk;
      out += chunk;
      continue;
    }

    // Compressed Block
    // 1. Read OrigSize (4 bytes little-endian)
    if (in + 4 > in_end)
      return 0;
    uint32_t orig_size = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
    in += 4;

    // 2. Read Tables
    // [Token:128][Lit:128][Off:128] = 384 bytes
    if (in + 384 > in_end)
      return 0;

    uint8_t token_lens[256];
    uint8_t lit_lens[256];
    uint8_t off_lens[256];

    for (int i = 0; i < 256; i += 2) {
      uint8_t b = *in++;
      token_lens[i] = (b >> 4);
      token_lens[i + 1] = (b & 0xF);
    }
    for (int i = 0; i < 256; i += 2) {
      uint8_t b = *in++;
      lit_lens[i] = (b >> 4);
      lit_lens[i + 1] = (b & 0xF);
    }
    for (int i = 0; i < 256; i += 2) {
      uint8_t b = *in++;
      off_lens[i] = (b >> 4);
      off_lens[i + 1] = (b & 0xF);
    }

    // Build Decoders
    zyphrax_huff_decoder token_dec, lit_dec, off_dec;
    zyphrax_build_dec_table(&token_dec, token_lens);
    zyphrax_build_dec_table(&lit_dec, lit_lens);
    zyphrax_build_dec_table(&off_dec, off_lens);

    // Init Reader
    z_bit_reader br = {.ptr = in, .end = in_end, .bit_buf = 0, .bit_count = 0};

    // Decode Loop - use orig_size for termination
    uint8_t *block_start = out;

    while ((size_t)(out - block_start) < orig_size) {
      // Safety check input
      if (br.ptr > br.end && br.bit_count < 8)
        break;

      // Decode Token
      uint8_t token = (uint8_t)decode_sym(&br, &token_dec);
      size_t t_ll = token >> 4;
      size_t t_ml = token & 0xF;

      // Lit Len
      size_t ll = t_ll;
      if (ll == 15)
        ll += read_start_extra(&br);

      // Copy Literals
      if (out + ll > out_end)
        return 0;
      for (size_t i = 0; i < ll; i++) {
        *out++ = (uint8_t)decode_sym(&br, &lit_dec);
      }

      if ((size_t)(out - block_start) >= orig_size)
        break;

      // Match - ml = t_ml + 3, plus extra if t_ml==15
      if (t_ml > 0) {
        size_t ml = t_ml + 3;
        if (t_ml == 15)
          ml += read_start_extra(&br);

        // Offset
        uint8_t off_hi = (uint8_t)decode_sym(&br, &off_dec);
        refill_bits(&br);
        uint8_t off_lo = (uint8_t)read_bits(&br, 8);
        uint16_t offset = (off_hi << 8) | off_lo;

        if (offset == 0)
          return 0; // Error

        // Execute Match
        if (out + ml > out_end)
          return 0;
        const uint8_t *match_src = out - offset;
        if (match_src < dst)
          return 0; // Underflow

        for (size_t k = 0; k < ml; k++) {
          out[k] = match_src[k];
        }
        out += ml;
      }
    }

    // Sync reader (approximate - advance to next block boundary)
    in = br.ptr;
  }

  return out - dst;
}
