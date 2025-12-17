#include "zyphrax.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE (50 * 1024 * 1024) // 50 MB
#define NUM_ITER 5

// Generate JSON-like data
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
  // Pad
  while (pos < size)
    buf[pos++] = ' ';
}

// Generate Binary (structured)
void gen_binary(uint8_t *buf, size_t size) {
  // Simulated sensor data: [id:4][ts:8][val:4][pad:16]
  struct {
    uint32_t id;
    uint64_t ts;
    uint32_t val;
    uint8_t pad[16];
  } record;

  size_t pos = 0;
  uint32_t id = 0;
  while (pos < size - sizeof(record)) {
    record.id = id++;
    record.ts = 1600000000 + id;
    record.val = (id * 17) % 10000;
    memset(record.pad, 0, 16);

    memcpy(buf + pos, &record, sizeof(record));
    pos += sizeof(record);
  }
}

// Generate Text (English-like)
void gen_text(uint8_t *buf, size_t size) {
  const char *words[] = {
      "the",         "quick",     "brown",      "fox",    "jumps",
      "over",        "lazy",      "dog",        "system", "compression",
      "performance", "benchmark", "throughput", "latency"};
  size_t num_words = sizeof(words) / sizeof(words[0]);
  size_t pos = 0;
  int i = 0;
  while (pos < size - 20) {
    const char *w = words[i % num_words];
    int len = strlen(w);
    memcpy(buf + pos, w, len);
    pos += len;
    buf[pos++] = ' ';
    i = (i * 3 + 7);
  }
  while (pos < size)
    buf[pos++] = '.';
}

void benchmark(const char *name, void (*gen)(uint8_t *, size_t)) {
  uint8_t *src = malloc(DATA_SIZE);
  if (!src) {
    fprintf(stderr, "Alloc fail\n");
    exit(1);
  }

  gen(src, DATA_SIZE);

  size_t bound = zyphrax_compress_bound(DATA_SIZE);
  uint8_t *dst = malloc(bound);
  if (!dst) {
    fprintf(stderr, "Alloc fail\n");
    exit(1);
  }

  zyphrax_params_t params = {.level = 3, .block_size = 65536};

  // Warmup
  zyphrax_compress(src, DATA_SIZE, dst, bound, &params);

  clock_t start = clock();
  size_t total_out = 0;

  for (int i = 0; i < NUM_ITER; i++) {
    total_out = zyphrax_compress(src, DATA_SIZE, dst, bound, &params);
  }

  clock_t end = clock();
  double secs = (double)(end - start) / CLOCKS_PER_SEC;
  double mb = (double)(DATA_SIZE * NUM_ITER) / (1024.0 * 1024.0);
  double speed = mb / secs;         // MB/s
  double gb_speed = speed / 1024.0; // GB/s
  double ratio = (double)DATA_SIZE / total_out;

  printf("| %-10s | ~%.2f GB/s       | %.2f : 1               |\n", name,
         gb_speed, ratio);

  free(src);
  free(dst);
}

int main() {
  printf("| Data Type | Compression Speed | Compression Ratio (Avg) |\n");
  printf("|-----------|-------------------|-------------------------|\n");

  benchmark("JSON Logs", gen_json);
  benchmark("Binary", gen_binary);
  benchmark("Text", gen_text);

  return 0;
}
