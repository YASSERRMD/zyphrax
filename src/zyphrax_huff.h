#pragma once
#include "zyphrax_seq.h" // For sequences analysis
#include <stddef.h>
#include <stdint.h>

// Max symbols for our alphabets
#define LIT_SYMBOLS 256
#define OFF_SYMBOLS 256 // Only encoding high byte of offset?
// Prompt says: "Offsets (high byte only, ~256 symbols)"
// Note: Low byte of offset is usually raw or part of stream.
#define MLEN_SYMBOLS 256 // 0-255 representing 4-259?

typedef struct {
  uint32_t freq[256];
  uint8_t code_len[256];
  uint16_t code[256];
} zyphrax_huffman_t;

// Bit Writer
typedef struct {
  uint64_t bits;  // Buffer for bits
  int count;      // Number of bits in buffer
  uint8_t *out;   // Current output pointer
  uint8_t *start; // Start of buffer (for offset calc)
  uint8_t *end;   // End of buffer
} zyphrax_bit_writer_t;

void zyphrax_bw_init(zyphrax_bit_writer_t *bw, uint8_t *buf, size_t cap);
void zyphrax_bw_put(zyphrax_bit_writer_t *bw, uint32_t value, int bits);
void zyphrax_bw_flush(zyphrax_bit_writer_t *bw);
size_t zyphrax_bw_written(const zyphrax_bit_writer_t *bw);

// Huffman Analysis & Build
// Analyze sequences to populate frequency counts for the 3 trees
void zyphrax_analyze_sequences(const zyphrax_sequence_t *seqs, size_t count,
                               zyphrax_huffman_t *lit_hf,
                               zyphrax_huffman_t *off_hf,
                               zyphrax_huffman_t *token_hf);

// Build tree from frequencies (generates code_len and code)
void zyphrax_build_huffman(zyphrax_huffman_t *hf);

// Encode sequences using the built trees
size_t zyphrax_huffman_encode(const zyphrax_sequence_t *seqs, size_t count,
                              uint8_t *dst, size_t dst_cap,
                              const zyphrax_huffman_t *lit_hf,
                              const zyphrax_huffman_t *off_hf,
                              const zyphrax_huffman_t *token_hf);
