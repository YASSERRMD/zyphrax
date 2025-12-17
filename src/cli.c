#include "zyphrax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHUNK_SIZE (1024 * 1024)

void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [-d] <input> <output>\n", prog);
}

int main(int argc, char **argv) {
  int decompress = 0;
  int arg_idx = 1;

  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[arg_idx], "-d") == 0) {
    decompress = 1;
    arg_idx++;
    if (argc < 4) {
      print_usage(argv[0]);
      return 1;
    }
  }

  const char *in_path = argv[arg_idx];
  const char *out_path = argv[arg_idx + 1];

  FILE *fin = fopen(in_path, "rb");
  if (!fin) {
    perror("Error opening input");
    return 1;
  }

  fseek(fin, 0, SEEK_END);
  long fsize = ftell(fin);
  fseek(fin, 0, SEEK_SET);

  if (fsize < 0) {
    fprintf(stderr, "Error input size\n");
    fclose(fin);
    return 1;
  }

  uint8_t *in_buf = malloc(fsize);
  if (!in_buf) {
    fprintf(stderr, "Memory error\n");
    fclose(fin);
    return 1;
  }
  if (fread(in_buf, 1, fsize, fin) != (size_t)fsize) {
    fprintf(stderr, "Read error\n");
    free(in_buf);
    fclose(fin);
    return 1;
  }
  fclose(fin);

  FILE *fout = fopen(out_path, "wb");
  if (!fout) {
    perror("Error opening output");
    free(in_buf);
    return 1;
  }

  clock_t start = clock();
  size_t out_sz = 0;

  if (!decompress) {
    // Compress
    size_t bound = zyphrax_compress_bound(fsize);
    uint8_t *out_buf = malloc(bound);
    if (!out_buf) {
      fprintf(stderr, "Out mem error\n");
      free(in_buf);
      fclose(fout);
      return 1;
    }

    zyphrax_params_t params = {.level = 3, .block_size = 65536};
    out_sz = zyphrax_compress(in_buf, fsize, out_buf, bound, &params);

    if (out_sz == 0 && fsize > 0) {
      fprintf(stderr, "Compression failed\n");
    } else {
      fwrite(out_buf, 1, out_sz, fout);
      double ratio = (double)out_sz * 100.0 / fsize;
      printf("Compressed %ld -> %zu bytes (%.2f%%)\n", fsize, out_sz, ratio);
    }
    free(out_buf);
  } else {
    // Decompress
    // We need to know original size?
    // Header doesn't store full original size, only block sizes.
    // Usually we decompress block by block.
    // For CLI, let's guess output size * 10 or params?
    // Zyphrax format doesn't have "Total Size" field in header.
    // So we might need incremental decompression API (Phase 9+).
    // Since we only have `zyphrax_decompress` doing oneshot (returning 0),
    // we can't really implement CLI decompress fully yet.
    // We will just call it and report result.

    size_t cap = fsize * 10; // Simple guess
    uint8_t *out_buf = malloc(cap);
    if (!out_buf) {
      fprintf(stderr, "Out mem error\n");
      free(in_buf);
      fclose(fout);
      return 1;
    }

    out_sz = zyphrax_decompress(in_buf, fsize, out_buf, cap);
    if (out_sz == 0) {
      fprintf(stderr,
              "Decompression not fully implemented or failed (Phase 9)\n");
    } else {
      fwrite(out_buf, 1, out_sz, fout);
      printf("Decompressed %ld -> %zu bytes\n", fsize, out_sz);
    }
    free(out_buf);
  }

  clock_t end = clock();
  double secs = (double)(end - start) / CLOCKS_PER_SEC;
  printf("Time: %.3fs\n", secs);

  free(in_buf);
  fclose(fout);
  return 0;
}
