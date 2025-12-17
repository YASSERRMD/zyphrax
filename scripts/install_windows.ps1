# Zyphrax Windows Installer (PowerShell)
# Requires MinGW (gcc) in PATH.

$ErrorActionPreference = "Stop"

Write-Host "Building Zyphrax for Windows..."

# Compile Objects
gcc -O3 -I src -c src/zyphrax.c -o src/zyphrax.o
gcc -O3 -I src -c src/zyphrax_lz77.c -o src/zyphrax_lz77.o
gcc -O3 -I src -c src/zyphrax_simd.c -o src/zyphrax_simd.o
gcc -O3 -I src -c src/zyphrax_seq.c -o src/zyphrax_seq.o
gcc -O3 -I src -c src/zyphrax_huff.c -o src/zyphrax_huff.o
gcc -O3 -I src -c src/zyphrax_block.c -o src/zyphrax_block.o
gcc -O3 -I src -c src/zyphrax_dec.c -o src/zyphrax_dec.o

# Static Lib
ar rcs libzyphrax.a src/zyphrax.o src/zyphrax_lz77.o src/zyphrax_simd.o src/zyphrax_seq.o src/zyphrax_huff.o src/zyphrax_block.o src/zyphrax_dec.o
Write-Host "Created libzyphrax.a"

# Shared Lib (DLL)
gcc -shared -o zyphrax.dll src/zyphrax.o src/zyphrax_lz77.o src/zyphrax_simd.o src/zyphrax_seq.o src/zyphrax_huff.o src/zyphrax_block.o src/zyphrax_dec.o
Write-Host "Created zyphrax.dll"

# CLI
gcc -O3 -I src src/cli.c -L. -lzyphrax -o zyphrax.exe
Write-Host "Created zyphrax.exe"

# Installation (Local)
# Windows doesn't have a standard /usr/local. 
# We just output instructions or copy to a user specified folder?
# For now, we leave them in root and user handles PATH.

Write-Host "Build Complete!"
Write-Host "Artifacts:"
Write-Host " - libzyphrax.a"
Write-Host " - zyphrax.dll"
Write-Host " - zyphrax.exe"
Write-Host " - src/zyphrax.h"
