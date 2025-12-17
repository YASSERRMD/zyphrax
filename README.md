# Zyphrax Compression Engine

<p align="center">
  <img src="assets/logo.png" alt="Zyphrax Logo" width="400">
</p>

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

### Environment Setup

After installation, ensure your system can find the shared library:

**Linux**
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

**macOS**
```bash
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:/usr/local/lib
```

**Windows**
Add the directory containing `zyphrax.dll` to your System `PATH`, or keep the DLL in the same folder as your executable.

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
    
    // Decompress
    size_t dec_bound = src_len; // Known size
    uint8_t *dec = malloc(dec_bound);
    size_t dec_size = zyphrax_decompress(dst, comp_size, dec, dec_bound);
    
    free(dst);
    free(dec);
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
use zyphrax::{compress, decompress, ZyphraxParams};

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
    
    // Decompress
    if let Some(original) = decompress(&compressed, data.len()) {
        println!("Decompressed size: {}", original.len());
    }
}
```

---

---

### Polyglot Usage

#### Python

```python
import zyphrax

data = b"Hello World"
# Compress
compressed = zyphrax.compress(data)
# Decompress
original = zyphrax.decompress(compressed)
```

#### Go

```go
package main

import (
    "fmt"
    "zyphrax"
)

func main() {
    data := []byte("Hello World")
    // Compress
    compressed, _ := zyphrax.Compress(data)
    // Decompress
    original, _ := zyphrax.Decompress(compressed, 0)
    fmt.Printf("%s\n", original)
}
```

#### C# (dotnet)

```csharp
using Zyphrax;

byte[] data = System.Text.Encoding.UTF8.GetBytes("Hello World");
// Compress
byte[] compressed = Compressor.Compress(data);
// Decompress
byte[] original = Compressor.Decompress(compressed);
```

#### TypeScript (Node.js)

```typescript
import { compress, decompress } from './bindings/typescript/zyphrax';

const data = Buffer.from("Hello World");
// Compress
const compressed = compress(data);
// Decompress
const original = decompress(compressed);
```

---

## Performance (Synthetic Benchmarks, Single-Threaded)

Timing measured using CPU-time over 5 iterations on 50 MB datasets (Apple M4 ARM64).

### Compression

| Data Type            | Speed      | Ratio       |
|----------------------|------------|-------------|
| JSON (Large)         | 0.41 GB/s  |  9.9 : 1    |
| Text (Large)         | 4.05 GB/s  | 62.9 : 1    |
| Binary (Large)       | 0.07 GB/s  |  3.4 : 1    |
| Random (Adversarial) | 0.08 GB/s  |  1.0 : 1    |
| JSON (4KB Blocks)    | 0.24 GB/s  |  4.9 : 1    |

### Decompression

| Data Type          | Speed      |
|--------------------|------------|
| JSON (Large)       | 0.99 GB/s  |

Decompression verified byte-for-byte against original input.

### Multi-threaded Scaling

| Configuration      | Throughput  |
|--------------------|-------------|
| 4 Threads (JSON)   | 1.29 GB/s   |

> **Note**: Performance varies by data type. Text data compresses extremely well (~63:1) due to high redundancy. Random data uses raw storage fallback (1:1 ratio) as expected.

---

## Target Use Cases

Zyphrax is **not** a general-purpose replacement for Zstd (which aims for higher ratios).
It is specifically designed for:

* **Ingestion Pipelines**: Where data throughput (>2 GB/s) is critical.
* **Structured Data**: JSON, XML, or binary protocols with repetitive headers/keys.
* **Low-Latency Storage**: Writing compressed blocks to disk/network deterministically.
* **Embedded/Constrained Systems**: Where simple memory management (fixed blocks) is preferred over complex streaming states.
