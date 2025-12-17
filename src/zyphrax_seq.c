#include "zyphrax_seq.h"
#include <string.h>

// Helper to write length with variable encoding
// returns number of bytes written
static size_t write_len(size_t val, uint8_t *p) {
  size_t n = 0;
  while (val >= 255) {
    p[n++] = 255;
    val -= 255;
  }
  p[n++] = (uint8_t)val;
  return n;
}

size_t zyphrax_encode_sequence(const zyphrax_sequence_t *seq, uint8_t *out,
                               size_t out_cap) {
  uint8_t *op = out;
  uint8_t *const oend = out + out_cap;

  // Bounds check reserved for minimal safety.
  // We assume caller guarantees reasonable space (approx seq->lit_len + 32).

  // 1. Calculate token lengths
  size_t ll = seq->lit_len;
  size_t ml = seq->match.length;

  // match_len in token is match_len - 4 (MIN_MATCH)
  // If no match (ml=0), we treat it as end of block if we are flushing
  // literals? But this function encodes a sequence (lit + match). If ml < 4, it
  // means we only have literals (last sequence). LZ4 spec: last sequence has
  // only literals. But format says: [lit_len:4][match_len:4]. If last sequence,
  // we can't emit match token parts. But let's follow the standard pattern: If
  // ml < 4, we assume it's the final literal sequence, match length ignores or
  // is 0? Standard LZ4 valid blocks must end with literals. Zyphrax simplified:
  // Sequence always has literals, maybe match. If match is invalid/empty, we
  // handle carefully.

  // Clamp lengths for token
  uint8_t token_ll = (ll >= 15) ? 15 : (uint8_t)ll;
  uint8_t token_ml = 0;
  if (ml >= 4) {
    token_ml = (ml - 4 >= 15) ? 15 : (uint8_t)(ml - 4);
  }

  // Write token
  if (op >= oend)
    return 0; // Error
  *op++ = (token_ll << 4) | token_ml;

  // Write extra lit length
  if (token_ll == 15) {
    if (op + (ll - 15) / 255 + 1 > oend)
      return 0;
    op += write_len(ll - 15, op);
  }

  // Copy literals
  if (op + ll > oend)
    return 0;
  memcpy(op, seq->literals, ll);
  op += ll;

  // If valid match, write offset and extra match length
  if (ml >= 4) {
    if (op + 2 > oend)
      return 0;
    // Offset is little endian u16
    // seq->match.offset is distance back
    uint16_t off = seq->match.offset;
    op[0] = off & 0xFF;
    op[1] = (off >> 8) & 0xFF;
    op += 2;

    if (token_ml == 15) {
      if (op + (ml - 4 - 15) / 255 + 1 > oend)
        return 0;
      op += write_len((ml - 4) - 15, op);
    }
  }

  return op - out;
}

size_t zyphrax_seq_bound(const zyphrax_sequence_t *seq) {
  // Token (1) + lit_len overhead + lit_len + offset (2) + match_len overhead
  size_t ll = seq->lit_len;
  size_t ml = seq->match.length;

  size_t bound = 1 + ll + 2;

  if (ll >= 15)
    bound += (ll - 15) / 255 + 1;
  if (ml >= 4 + 15)
    bound += (ml - 19) / 255 + 1;

  return bound;
}
