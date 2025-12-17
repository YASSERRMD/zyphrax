#include "zyphrax_block.h"
#include "zyphrax_huff.h"
#include "zyphrax_lz77.h"
#include "zyphrax_seq.h"
#include <stdlib.h>
#include <string.h>

// Helper to store raw block
static size_t zyphrax_store_raw(const uint8_t *src, size_t src_size,
                                uint8_t *dst, size_t dst_cap) {
  if (dst_cap < src_size + 4)
    return 0; // Need space for raw header?
  // Actually, block header is separate?
  // "Phase 1: Header (12 bytes) ... Process blocks... Header read/write"
  // Phase 6 doc: "Main Compression Loop ... zyphrax_compress_block"
  // But how do we distinguish compressed vs raw block in the output stream?
  // The main Frame Header has `raw_block:1`. That's for the WHOLE FRAME maybe?
  // Or do blocks have headers?
  // "Block Format: [Hdr][LitTree]..."
  // "Hdr" likely contains block type (Compressed/Raw) + Size.
  // Let's assume a 1-byte or 3-byte block header.
  // Spec in Phase 5: "[Hdr][LitTree]..."
  // Let's define Block Header:
  // [BlockSize:16? or 24?][Type:1?]
  // If raw, we just dump bytes?
  // But the caller iterates blocks.
  // If returning size, the caller usually frames it?
  // The prompt trace in Phase 7 says:
  // "Process blocks... out += block_enc;"
  // It doesn't write a block header explicitly in Phase 7 trace...
  // Wait, Phase 5 said: "Block Format: [Hdr][LitTree]..."
  // So `zyphrax_compress_block` MUST write the header.

  // Let's define simple block header:
  // Byte 0: Type (0=Raw, 1=Compressed)
  // Byte 1-3: Compressed Size (if needed? or implied?)
  // If we return size, the containing format might need lengths.
  // Let's assume format is:
  // [Type][Data...]

  // Raw: [Type=0][RawBytes...]

  if (dst_cap < src_size + 1)
    return 0;
  dst[0] = 0; // Raw
  memcpy(dst + 1, src, src_size);
  return src_size + 1;
}

#define MAX_SEQS                                                               \
  (64 * 1024 / 4) // Worst case: 4 byte matches? Or just literals?
// If all literals, experimental: 1 sequence per block?
// LZ77 produces sequences.
// A sequence has `literals` pointer and `lit_len`.
// If no matches, 1 sequence with lit_len = block_size.
// So max sequences is block_size/MIN_MATCH roughly, if many small matches.
// 64K / 3 = ~21K sequences.
// Let's allocate on stack or static? 21K * sizeof(seq) = 21K * 24 bytes =
// 500KB. Too big for stack. Use malloc.

size_t zyphrax_compress_block(const uint8_t *src, size_t src_size, uint8_t *dst,
                              size_t dst_cap,
                              const zyphrax_params_t *params // Unused for now
) {
  if (src_size == 0)
    return 0;

  // 1. LZ77
  // Need state. Large struct (256K+128K).
  // Allocate on heap.
  zyphrax_lz77_t *lz = malloc(sizeof(zyphrax_lz77_t));
  if (!lz)
    return 0;

  zyphrax_lz77_init(lz);

  // Sequence buffer
  size_t max_seqs = (src_size / 4) + 256;
  // Wait, MIN_MATCH=4.
  if (max_seqs < 1024)
    max_seqs = 1024;

  zyphrax_sequence_t *seqs = malloc(max_seqs * sizeof(zyphrax_sequence_t));
  if (!seqs) {
    free(lz);
    return 0;
  }

  size_t seq_count = 0;
  size_t pos = 0;
  size_t lit_start = 0;

  while (pos < src_size) {
    // Find match
    zyphrax_match_t m = zyphrax_find_best_match(lz, src, pos, src_size);

    if (m.length >= MIN_MATCH) {
      // Found match
      // Emit sequence
      if (seq_count >= max_seqs) {
        // Buffer full? Realloc?
        // Just break and encode what we have? Or raw?
        // Should be rare if sized correctly.
        // Treat as raw fallback.
        free(lz);
        free(seqs);
        return zyphrax_store_raw(src, src_size, dst, dst_cap);
      }

      zyphrax_sequence_t *s = &seqs[seq_count++];
      s->literals = src + lit_start;
      s->lit_len = pos - lit_start;
      s->match = m;

      pos += m.length;
      lit_start = pos;
    } else {
      pos++;
    }
  }

  // Final sequence (literals only)
  if (lit_start < src_size) {
    if (seq_count < max_seqs) {
      zyphrax_sequence_t *s = &seqs[seq_count++];
      s->literals = src + lit_start;
      s->lit_len = src_size - lit_start;
      s->match.length = 0;
      s->match.offset = 0;
    }
  }

  free(lz); // Done with LZ77

  // 2. Freq Analysis
  zyphrax_huffman_t lit_hf, off_hf, token_hf;
  zyphrax_analyze_sequences(seqs, seq_count, &lit_hf, &off_hf, &token_hf);

  // 3. Build Trees
  zyphrax_build_huffman(&lit_hf);
  zyphrax_build_huffman(&off_hf);
  zyphrax_build_huffman(&token_hf);

  // 4. Encode
  // Header: [Type:1][OrigSize:4][Data...]
  if (dst_cap < 5) {
    free(seqs);
    return 0;
  }
  dst[0] = 1; // Compressed
  // Write original size (little-endian u32)
  dst[1] = (uint8_t)(src_size & 0xFF);
  dst[2] = (uint8_t)((src_size >> 8) & 0xFF);
  dst[3] = (uint8_t)((src_size >> 16) & 0xFF);
  dst[4] = (uint8_t)((src_size >> 24) & 0xFF);

  size_t written = zyphrax_huffman_encode(seqs, seq_count, dst + 5, dst_cap - 5,
                                          &lit_hf, &off_hf, &token_hf);

  free(seqs);

  if (written == 0 || written + 5 >= src_size) {
    // Fallback to raw
    return zyphrax_store_raw(src, src_size, dst, dst_cap);
  }

  return written + 5;
}
