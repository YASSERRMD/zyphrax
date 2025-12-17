# Zyphrax Compression Engine

Zyphrax is a high-performance, block-based compression engine designed for **high-throughput structured data pipelines**. It combines an LZ77 core, SIMD-accelerated match finding, and lightweight Huffman entropy coding in a predictable block format.

Zyphrax is optimized for **speed, low latency, and deterministic behavior**, positioning it between LZ4-class compressors and fast Zstd modes.

---

## Features

* **High Throughput Core**
  * SIMD-accelerated LZ77 match finding (AVX2 / NEON)
  * Optimized for large, contiguous inputs
* **Competitive Compression Ratio**
  * LZ77 + per-block Huffman coding
  * Strong gains on structured and semi-structured data
* **Predictable Block Format**
  * Fixed-size blocks (default 64 KB)
  * No adaptive heuristics or auto-tuning
* **Safe APIs**
  * Clean C API
  * Zero-cost Rust FFI wrapper
  * Bindings for Python, Go, C# and TypeScript
* **Portability**
  * x86_64 (Linux, macOS)
  * ARM64 (macOS, Linux)

---

## Build Instructions

### C Library

```bash
# Build static library
gcc -O3 -march=native -Wall -Wextra -I src -c src/*.c
ar rcs libzyphrax.a *.o

# Build shared library
make shared

# Run tests
./tests/test_api
```

> Note: SIMD paths are enabled automatically based on compiler flags.

---

### Rust Crate

```bash
cargo build --release
cargo test
```

---

## Installation

### Linux / macOS

Use the provided install script to build and install globally (requires sudo):

```bash
chmod +x scripts/install_unix.sh
./scripts/install_unix.sh
```

By default, it installs to `/usr/local` (lib, include, bin).

### Windows

Run the PowerShell script (requires MinGW/GCC in PATH):

```powershell
./scripts/install_windows.ps1
```

This creates `zyphrax.dll`, `libzyphrax.a`, and `zyphrax.exe` in the project root.

---

## Usage

### C API

```c
#include "zyphrax.h"

void example() {
    const uint8_t *src = ...;
    size_t src_len = ...;

    size_t bound = zyphrax_compress_bound(src_len);
    uint8_t *dst = malloc(bound);

    zyphrax_params_t params = {
        .level = 3,
        .block_size = 64 * 1024,
        .checksum = 1
    };

    size_t comp_size = zyphrax_compress(
        src, src_len,
        dst, bound,
        &params
    );

    printf("Compressed size: %zu bytes\n", comp_size);
    free(dst);
}
```

---

### Rust API

Add to `Cargo.toml`:

```toml
[dependencies]
zyphrax = { path = "path/to/zyphrax" }
```

```rust
use zyphrax::{compress, ZyphraxParams};

fn main() {
    let data = b"Hello World";

    let params = ZyphraxParams {
        level: 3,
        block_size: 64 * 1024,
        checksum: 1,
    };

    // Pass params by value (Copy trait)
    let compressed = compress(data, Some(params));
    println!("Compressed size: {}", compressed.len());
}
```

---

### Polyglot Bindings

- [Python](bindings/python/)
- [Go](bindings/go/)
- [C#](bindings/csharp/)
- [TypeScript](bindings/typescript/)

---

## Architecture Status

* **Phase 1**: Block Format & Header — Completed
* **Phase 2**: LZ77 Match Finder — Completed
* **Phase 3**: SIMD Acceleration — Completed
* **Phase 4**: Token Encoding — Completed
* **Phase 5**: Huffman Coding — Encoder completed
* **Phase 6**: Block Compressor — Completed
* **Phase 7**: C API — Completed
* **Phase 8**: Rust Wrapper — Completed
* **Phase 9**: Decompression Core — Table builder implemented
* **Phase 10**: CLI & Polish — Completed
* **Phase 11**: Polyglot Bindings — Completed

---

## Positioning

Zyphrax is **not** a general-purpose replacement for Zstd.
It is designed for:
* High-throughput ingestion pipelines
* Structured and semi-structured data
* Low-latency block compression
* Systems where predictability matters more than maximum ratio
