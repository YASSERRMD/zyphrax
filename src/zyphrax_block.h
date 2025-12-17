#pragma once
#include "zyphrax.h"
#include <stddef.h>
#include <stdint.h>

// Compresses a single block (up to 64KB or whatever params say)
// Returns compressed size.
// If compressed size >= src_size (expansion), returns 0 or flag?
// Prompt says: "4. Raw fallback if worse. return zyphrax_store_raw..."
// We'll return size.
size_t zyphrax_compress_block(const uint8_t *src, size_t src_size, uint8_t *dst,
                              size_t dst_cap, const zyphrax_params_t *params);
