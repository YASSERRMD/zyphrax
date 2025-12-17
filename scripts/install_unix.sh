#!/bin/bash
set -e

# Zyphrax Unix Installer (Linux/macOS)

PREFIX=${PREFIX:-/usr/local}
LIB_DIR="$PREFIX/lib"
INC_DIR="$PREFIX/include"
BIN_DIR="$PREFIX/bin"

echo "Building Zyphrax..."
make clean
make all
make shared

echo "Installing to $PREFIX..."

# Check permissions
if [ ! -w "$LIB_DIR" ]; then
    echo "Need sudo access to write to $PREFIX"
    SUDO="sudo"
else
    SUDO=""
fi

$SUDO mkdir -p "$LIB_DIR" "$INC_DIR" "$BIN_DIR"

# Install Libs
if [ -f "libzyphrax.dylib" ]; then
    $SUDO cp libzyphrax.dylib "$LIB_DIR/"
    echo "Installed libzyphrax.dylib"
elif [ -f "libzyphrax.so" ]; then
    $SUDO cp libzyphrax.so "$LIB_DIR/"
    echo "Installed libzyphrax.so"
fi

$SUDO cp libzyphrax.a "$LIB_DIR/"
echo "Installed libzyphrax.a"

# Install Header
$SUDO cp src/zyphrax.h "$INC_DIR/"
echo "Installed zyphrax.h"

# Install CLI
$SUDO cp zyphrax "$BIN_DIR/"
echo "Installed zyphrax CLI"

echo "Installation Complete!"
echo "You may need to run 'ldconfig' (Linux) or update DYLD_LIBRARY_PATH (macOS) if not in standard path."
