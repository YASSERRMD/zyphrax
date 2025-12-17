CC = gcc
CFLAGS = -O3 -I src

# Detect Arch forflags
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    CFLAGS += -mavx2 -mbmi2
endif
# ARM64 usually implied or -march=native

SRC_LIB = src/zyphrax.c src/zyphrax_lz77.c src/zyphrax_simd.c src/zyphrax_seq.c src/zyphrax_huff.c src/zyphrax_block.c src/zyphrax_dec.c
OBJ_LIB = $(SRC_LIB:.c=.o)

all: lib cli

lib: $(OBJ_LIB)
	ar rcs libzyphrax.a $(OBJ_LIB)

cli: lib src/cli.c
	$(CC) $(CFLAGS) src/cli.c -L. -lzyphrax -o zyphrax

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: lib
	$(CC) $(CFLAGS) tests/test_header.c -L. -lzyphrax -o tests/test_header
	$(CC) $(CFLAGS) tests/test_lz77.c src/zyphrax_simd.c -o tests/test_lz77
	$(CC) $(CFLAGS) tests/test_simd.c -o tests/test_simd
	$(CC) $(CFLAGS) tests/test_tokens.c src/zyphrax_seq.c -o tests/test_tokens
	$(CC) $(CFLAGS) tests/test_huffman.c src/zyphrax_huff.c -o tests/test_huffman
	$(CC) $(CFLAGS) tests/test_block.c -L. -lzyphrax -o tests/test_block
	$(CC) $(CFLAGS) tests/test_api.c -L. -lzyphrax -o tests/test_api
	$(CC) $(CFLAGS) tests/test_decompress.c src/zyphrax_dec.c  -o tests/test_decompress
	./tests/test_header
	./tests/test_lz77
	./tests/test_simd
	./tests/test_tokens
	./tests/test_huffman
	./tests/test_block
	./tests/test_api
	./tests/test_decompress

clean:
	rm -f src/*.o libzyphrax.a zyphrax
	rm -f tests/test_header tests/test_lz77 tests/test_simd tests/test_tokens tests/test_huffman tests/test_block tests/test_api tests/test_decompress
