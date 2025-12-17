#pragma once
#include <stddef.h>
#include <stdint.h>

// Huffman Decoder Table
// For fast decoding, we use a lookup table.
// Max code length 15.
// A simple table of size 2^MAX_LEN? Too big? No, 2^15 = 32KB entries.
// Each entry maps bits -> symbol + length.
// 2^15 * 2 bytes = 64KB per table.
// We have 3 tables (Lit, Off, MLen). Total 192KB stack/mem.
// Acceptable for modern decompression context.
// Or we can use multi-level (e.g. 2^10 + 2^5).
// For simplicity and speed (3GB/s goal), single table is best if cache allows.
// We'll use 2 levels: 1st table 4096 (12 bits), 2nd level for >12.
// OR just full table 2^15?
// Let's stick to full table if memory allows.
// "Deliverable: 3+ GB/s".
// We'll default to 2^11 (2048) and sub-tables? Or direct.
// Let's implement full table construction for now but optimized for 12 bit?
// 15 bits is fine.

#define HUFF_MAX_BITS 15
#define HUFF_TABLE_BITS 12 // Primary table size
#define HUFF_LOOKUP_SIZE (1 << HUFF_TABLE_BITS)

typedef struct {
  uint8_t symbol; // 0-255
  uint8_t bits;   // Length of code
} zyphrax_dec_entry;

typedef struct {
  zyphrax_dec_entry lookup[HUFF_LOOKUP_SIZE];
  // For codes > 12 bits, we need extensions.
  // Implementing simpler variant: Full 15 bit table uses 32K entries.
  // 32K * 2 bytes = 64KB. This is small.
  // Let's use 15 bit direct table for max speed.
} zyphrax_dec_table;

#define FAST_TABLE_BITS 15
#define FAST_TABLE_SIZE (1 << FAST_TABLE_BITS)

typedef struct {
  uint16_t table[FAST_TABLE_SIZE];
  // Entry format: [bits: 4][symbol: 12] (since bits <= 15)
  // Or [symbol:8][bits:8].
} zyphrax_huff_decoder;

void zyphrax_build_dec_table(zyphrax_huff_decoder *dec,
                             const uint8_t *code_lens);
