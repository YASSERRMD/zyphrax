#include "zyphrax_dec.h"
#include <string.h>

void zyphrax_build_dec_table(zyphrax_huff_decoder *dec,
                             const uint8_t *code_lens) {
  // Standard Canonical Huffman Dec Table Build
  // code_lens is array of 256 lengths.

  // 1. Count
  uint16_t bl_count[16] = {0};
  for (int i = 0; i < 256; i++) {
    bl_count[code_lens[i]]++;
  }

  // 2. Next Code
  uint16_t next_code[16];
  uint16_t code = 0;
  bl_count[0] = 0;
  for (int bits = 1; bits <= 15; bits++) {
    code = (code + bl_count[bits - 1]) << 1;
    next_code[bits] = code;
  }

  // 3. Fill Table
  // Init table with 0 (invalid)
  memset(dec->table, 0, sizeof(dec->table));

  for (int i = 0; i < 256; i++) {
    int len = code_lens[i];
    if (len == 0)
      continue;

    uint16_t c = next_code[len];
    next_code[len]++;

    // Reverse bits?
    // My encoder used MSB-first bit packing in `bw_put`.
    // `bw_put` implementation:
    // bw->bits |= ((uint64_t)value << bw->count);
    // This packs LSB-first into the byte stream (Value's LSB -> Byte's LSB).
    // Wait, `bw_put`:
    // value &= (1 << bits) - 1;
    // bits |= value << count;
    // This is standard LSB packing (Deflate style).
    // BUT my Huffman Code generation:
    // next_code logic `code = (code + bl_count[bits-1]) << 1` generates codes
    // where numeric value is standard. Usually Huffman codes are MSB-first in
    // the bitstream (for Deflate). But if I put them into an LSB bit reservoir
    // "as is", I must be careful. If I write `code` (e.g. 101) into LSB stream:
    // Stream: [1 0 1 ...]
    // Decoder reads LSB.
    // The table must index by the *reversed* code if I act like Deflate?
    // Let's assume my encoder wrote `value` directly. `value` is the code.
    // If code 6 (110) len 3.
    // Writer: puts 110 (binary 6).
    // Stream bits: 0 1 1. (LSB first).
    // Decoder peeks bits: sees 0, 1, 1.
    // Index is 011... = 3? No.
    // I need to reverse the bits of `c` to get the table index?
    // Yes, for LSB bit reader, the index is `reverse(code)`.

    // Bit Reverse `c` of length `len`.
    uint16_t r = 0;
    uint16_t tmp = c;
    for (int k = 0; k < len; k++) {
      r = (r << 1) | (tmp & 1);
      tmp >>= 1;
    }

    // Fill all entries with this prefix
    // The table index is the lookahead.
    // Code is `r` (reversed) in LSB stream.
    // We fill entries `idx` where `idx & mask == r`.
    // Stride is 1 << len.
    int stride = 1 << len;

    uint16_t entry = (i << 8) | len; // [sym:8][len:8]

    for (int j = r; j < FAST_TABLE_SIZE; j += stride) {
      dec->table[j] = entry;
    }
  }
}
