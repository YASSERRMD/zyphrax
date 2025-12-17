#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ZYPHRAX_MAGIC 0x58594659u  // "ZYFX" little-endian
#define ZYPHRAX_BLOCK_SIZE (64 << 10)  // 64KB

typedef struct {
    uint32_t level;      // 1-9
    uint32_t block_size; // 64KB default
    uint32_t checksum;   // CRC type: 0=None, 1=Adler32, 2=xxHash32 (impl specific)
} zyphrax_params_t;

// Returns the maximum compressed size for a given input size
size_t zyphrax_compress_bound(size_t src_size);

// Compresses data into the destination buffer
// Returns compressed size, or 0 on error (e.g. dst_cap too small)
size_t zyphrax_compress(const uint8_t *src, size_t src_size,
                        uint8_t *dst, size_t dst_cap,
                        const zyphrax_params_t *params);

// Decompresses data into the destination buffer
// Returns decompressed size, or 0 on error
size_t zyphrax_decompress(const uint8_t *src, size_t src_size,
                          uint8_t *dst, size_t dst_cap);
