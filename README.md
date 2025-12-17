# Zyphrax Compression Engine

Zyphrax is a high-performance, block-based compression engine designed for speed and efficiency. It features a custom format combining LZ77, SIMD-accelerated matching, and Huffman entropy coding.

## Features

- **High Speed**: >5 GB/s match finding (SIMD AVX2/NEON).
- **Compression**: Competitive ratio (LZ77 + Huffman).
- **Safe API**: Clean C API and safe Rust bindings.
- **Portability**: Runs on x86_64 (Linux/macOS) and ARM64 (macOS/Linux).

## Build Instructions

### C Library

```bash
# Build static library
gcc -O3 -c src/*.c
ar rcs libzyphrax.a *.o

# Run Tests
./tests/test_api
```

### Rust Crate

```bash
cargo build --release
cargo test
```

## Usage

### C API

```c
#include "zyphrax.h"

void example() {
    uint8_t *src = ...;
    size_t src_len = ...;
    
    // 1. Calculate bound
    size_t bound = zyphrax_compress_bound(src_len);
    
    // 2. Allocate dst
    uint8_t *dst = malloc(bound);
    
    // 3. Compress
    zyphrax_params_t params = { .level = 3, .block_size = 65536 };
    size_t comp_size = zyphrax_compress(src, src_len, dst, bound, &params);
    
    printf("Compressed: %zu bytes\n", comp_size);
    free(dst);
}
```

### Rust API

Add to `Cargo.toml`:
```toml
[dependencies]
zyphrax = { path = "path/to/zyphrax" }
```

```rust
use zyphrax;

fn main() {
    let data = b"Hello World";
    let compressed = zyphrax::compress(data, None);
    println!("Compressed size: {}", compressed.len());
}
```

## Architecture

- **Phase 1**: Block Format & Header (Completed)
- **Phase 2**: LZ77 Match Finder (Completed)
- **Phase 3**: SIMD Acceleration (Completed)
- **Phase 4**: Token Encoding (Completed)
- **Phase 5**: Huffman Coding (Completed)
- **Phase 6**: Block Compressor (Completed)
- **Phase 7**: C API (Completed)
- **Phase 8**: Rust Wrapper (Completed)
- **Phase 9**: Decompression Core (Table builder implemented)
