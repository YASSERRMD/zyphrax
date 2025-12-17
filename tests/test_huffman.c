#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/zyphrax_huff.c"

void test_analysis() {
  zyphrax_sequence_t seqs[1];
  uint8_t lit[] = "ABBA";
  seqs[0].literals = lit;
  seqs[0].lit_len = 4;
  seqs[0].match.length = 0;

  zyphrax_huffman_t lit_hf, off_hf, mlen_hf;
  zyphrax_analyze_sequences(seqs, 1, &lit_hf, &off_hf, &mlen_hf);

  assert(lit_hf.freq['A'] == 2);
  assert(lit_hf.freq['B'] == 2);
  assert(lit_hf.freq['C'] == 0);

  printf("Analysis test passed.\n");
}

void test_build_and_encode() {
  zyphrax_huffman_t hf = {0};
  hf.freq['A'] = 10;
  hf.freq['B'] = 1;
  hf.freq['C'] = 1;
  hf.freq['D'] = 1;

  zyphrax_build_huffman(&hf);

  // A should have shortest code (e.g. 1 bit: 0 or 1)
  // B,C,D longer.
  printf("Code A: len=%d val=%x\n", hf.code_len['A'], hf.code['A']);
  printf("Code B: len=%d val=%x\n", hf.code_len['B'], hf.code['B']);

  assert(hf.code_len['A'] < hf.code_len['B']);
  assert(hf.code_len['B'] > 0);

  // Test Bit Writer
  uint8_t buf[16] = {0};
  zyphrax_bit_writer_t bw;
  zyphrax_bw_init(&bw, buf, 16);

  // Write 0xFF (8 bits)
  zyphrax_bw_put(&bw, 0xFF, 8);
  // Write 0x0  (1 bit)
  zyphrax_bw_put(&bw, 0, 1);

  zyphrax_bw_flush(&bw);

  // Expect: 1 1 1 1 1 1 1 1 | 0 -> [FF] [00]? or [FF] [byte with 1 bit 0]?
  // Wait, bit writer approach:
  // put(0xFF, 8) -> bits=0xFF, count=8 -> flush byte 0xFF. bits=0, count=0.
  // put(0, 1) -> bits=0, count=1.
  // flush -> bits=0, count=1 -> write byte (0 & FF) = 00.
  // Written 2 bytes.
  assert(buf[0] == 0xFF);
  assert(buf[1] == 0x00);
  assert(zyphrax_bw_written(&bw) == 2);

  printf("Build & Encode test passed.\n");
}

int main() {
  test_analysis();
  test_build_and_encode();
  return 0;
}
