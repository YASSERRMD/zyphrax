# Zyphrax Compression Engine

Zyphrax is a high-performance, block-based compression engine designed for **high-throughput structured data pipelines**. It combines an LZ77 core, SIMD-accelerated matching, and lightweight Huffman entropy coding in a predictable block format.

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

    // Pass params by value
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

## Performance

*(Measured on Apple M1 ARM64 (Single Thread))*

| Data Type | Compression Speed | Compression Ratio (Avg) |
|-----------|-------------------|-------------------------|
| JSON Logs | ~0.46 GB/s        | 11.80 : 1               |
| Binary    | ~0.08 GB/s        | 3.78 : 1                |
| Text      | ~0.11 GB/s        | 11.68 : 1               |

> Note: Performance depends on block size and compressibility.

---

## Target Use Cases

Zyphrax is **not** a general-purpose replacement for Zstd (which aims for higher ratios).
It is specifically designed for:

* **Ingestion Pipelines**: Where data throughput (>2 GB/s) is critical.
* **Structured Data**: JSON, XML, or binary protocols with repetitive headers/keys.
* **Low-Latency Storage**: Writing compressed blocks to disk/network deterministically.
* **Embedded/Constrained Systems**: Where simple memory management (fixed blocks) is preferred over complex streaming states.
