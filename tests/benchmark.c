#include "zyphrax.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE (50 * 1024 * 1024) // 50 MB
#define NUM_ITER 5
#define SMALL_BLOCK_SIZE 4096

// Generators
void gen_json(uint8_t *buf, size_t size) {
  const char *template =
      "{\"id\":%d,\"name\":\"user_%d\",\"active\":true,\"roles\":[\"admin\","
      "\"editor\"],\"meta\":{\"ip\":\"192.168.1.%d\",\"ts\":%ld}},";
  size_t pos = 0;
  int i = 0;
  while (pos < size - 200) {
    int w = sprintf((char *)buf + pos, template, i, i, i % 255, (long)i * 1000);
    pos += w;
    i++;
  }
  while (pos < size)
    buf[pos++] = ' ';
}

void gen_binary(uint8_t *buf, size_t size) {
  struct {
    uint32_t id;
    uint64_t ts;
    uint32_t val;
    uint8_t pad[16];
  } rec;
  size_t pos = 0;
  uint32_t id = 0;
  while (pos < size - sizeof(rec)) {
    rec.id = id++;
    rec.ts = 1600000000 + id;
    rec.val = (id * 17) % 10000;
    memset(rec.pad, 0, 16);
    memcpy(buf + pos, &rec, sizeof(rec));
    pos += sizeof(rec);
  }
}

void gen_text(uint8_t *buf, size_t size) {
  const char *w[] = {"the",  "quick", "brown", "fox",    "jumps",
                     "over", "lazy",  "dog",   "system", "compression"};
  size_t pos = 0;
  int i = 0;
  while (pos < size - 20) {
    const char *wd = w[i % 10];
    int l = strlen(wd);
    memcpy(buf + pos, wd, l);
    pos += l;
    buf[pos++] = ' ';
    i++;
  }
  while (pos < size)
    buf[pos++] = '.';
}

void gen_random(uint8_t *buf, size_t size) {
  for (size_t i = 0; i < size; i++)
    buf[i] = rand() % 256;
}

// Benchmark Function
void bench_compress(const char *name, void (*gen)(uint8_t *, size_t),
                    int small_blocks) {
  uint8_t *src = malloc(DATA_SIZE);
  gen(src, DATA_SIZE);

  size_t bound = zyphrax_compress_bound(DATA_SIZE);
  if (small_blocks)
    bound = zyphrax_compress_bound(SMALL_BLOCK_SIZE) *
            (DATA_SIZE / SMALL_BLOCK_SIZE + 1);

  uint8_t *dst = malloc(bound);
  zyphrax_params_t params = {
      .level = 3, .block_size = small_blocks ? SMALL_BLOCK_SIZE : 65536};

  // Warmup
  if (!small_blocks)
    zyphrax_compress(src, DATA_SIZE, dst, bound, &params);

  clock_t start = clock();
  size_t total_out = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    if (small_blocks) {
      size_t pos = 0;
      size_t out_pos = 0;
      while (pos < DATA_SIZE) {
        size_t chunk = (DATA_SIZE - pos < SMALL_BLOCK_SIZE) ? DATA_SIZE - pos
                                                            : SMALL_BLOCK_SIZE;
        size_t w = zyphrax_compress(src + pos, chunk, dst + out_pos,
                                    bound - out_pos, &params);
        pos += chunk;
        out_pos += w;
      }
      total_out = out_pos;
    } else {
      total_out = zyphrax_compress(src, DATA_SIZE, dst, bound, &params);
    }
  }

  clock_t end = clock();
  double secs = (double)(end - start) / CLOCKS_PER_SEC;
  double speed = ((double)DATA_SIZE * NUM_ITER) / (1024 * 1024 * 1024.0) / secs;
  double ratio = (double)DATA_SIZE / total_out;

  printf("| %-18s | %5.2f GB/s | %5.2f : 1 |\n", name, speed, ratio);

  free(src);
  free(dst);
}

void bench_decompress() {
  printf("Benchmarking Decompression (JSON)...\n");
  // Generate JSON
  uint8_t *src = malloc(DATA_SIZE);
  gen_json(src, DATA_SIZE);

  size_t bound = zyphrax_compress_bound(DATA_SIZE);
  uint8_t *comp = malloc(bound);
  zyphrax_params_t params = {3, 65536, 0};
  size_t csz = zyphrax_compress(src, DATA_SIZE, comp, bound, &params);
  printf("Compressed Size: %zu\n", csz);

  uint8_t *dec = malloc(DATA_SIZE * 2);

  // Integrity Check
  size_t dsz_check = zyphrax_decompress(comp, csz, dec, DATA_SIZE * 2);
  if (dsz_check != DATA_SIZE || memcmp(src, dec, DATA_SIZE) != 0) {
    printf("Integrity Check FAILED! Decompressed size: %zu\n", dsz_check);
    free(src);
    free(comp);
    free(dec);
    return;
  }
  printf("Integrity Check Passed.\n");

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (int i = 0; i < NUM_ITER; i++) {
    zyphrax_decompress(comp, csz, dec, DATA_SIZE * 2);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  double secs =
      (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  double speed = ((double)DATA_SIZE * NUM_ITER) / (1024 * 1024 * 1024.0) / secs;

  printf("| %-18s | %5.2f GB/s |     -       |\n", "Decompression (JSON)",
         speed);

  free(src);
  free(comp);
  free(dec);
}

// Multithreaded Test
typedef struct {
  uint8_t *src;
  uint8_t *dst;
  size_t size;
  size_t bound;
  zyphrax_params_t p;
} thread_arg_t;

void *thread_func(void *arg) {
  thread_arg_t *a = (thread_arg_t *)arg;
  for (int i = 0; i < 5; i++) {
    zyphrax_compress(a->src, a->size, a->dst, a->bound, &a->p);
  }
  return NULL;
}

void bench_threads() {
  int nlines = 4;
  pthread_t th[4];
  thread_arg_t args[4];

  uint8_t *src = malloc(DATA_SIZE);
  gen_json(src, DATA_SIZE);

  for (int i = 0; i < nlines; i++) {
    args[i].src = src; // Read only
    args[i].size = DATA_SIZE;
    args[i].bound = zyphrax_compress_bound(DATA_SIZE);
    args[i].dst = malloc(args[i].bound);
    args[i].p.level = 3;
    args[i].p.block_size = 65536;
    args[i].p.checksum = 0;
  }

  clock_t start = clock(); // Note: clock() measures CPU time, wall time is
                           // needed for threads scaling
  // Use timespec
  struct timespec ts_start, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_start);

  for (int i = 0; i < nlines; i++)
    pthread_create(&th[i], NULL, thread_func, &args[i]);
  for (int i = 0; i < nlines; i++)
    pthread_join(th[i], NULL);

  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double secs = (ts_end.tv_sec - ts_start.tv_sec) +
                (ts_end.tv_nsec - ts_start.tv_nsec) / 1e9;

  // Total throughput = 4 threads * 5 iters * DATA_SIZE
  double total_bytes = (double)nlines * 5 * DATA_SIZE;
  double speed = total_bytes / (1024 * 1024 * 1024.0) / secs;

  printf("| %-18s | %5.2f GB/s |     -       |\n", "Thread Scale (x4)", speed);

  free(src);
  for (int i = 0; i < nlines; i++)
    free(args[i].dst);
}

int main() {
  printf("| Benchmark Category   | Speed      | Ratio       |\n");
  printf("|----------------------|------------|-------------|\n");
  bench_compress("JSON (Large)", gen_json, 0);
  bench_compress("Binary (Large)", gen_binary, 0);
  bench_compress("Text (Large)", gen_text, 0);
  bench_compress("Random (Adversarial)", gen_random, 0);
  bench_compress("JSON (4KB Blocks)", gen_json, 1);
  bench_decompress();
  bench_threads();
  return 0;
}
