package zyphrax

/*
#cgo CFLAGS: -I../../src
#cgo LDFLAGS: -L../../ -lzyphrax
#include "zyphrax.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"unsafe"
)

func Compress(data []byte) ([]byte, error) {
	if len(data) == 0 {
		return nil, nil
	}

	cSrc := (*C.uchar)(unsafe.Pointer(&data[0]))
	cLen := C.size_t(len(data))

	bound := C.zyphrax_compress_bound(cLen)
	dst := make([]byte, bound)
	cDst := (*C.uchar)(unsafe.Pointer(&dst[0]))

	// Create params on stack (C struct)
	var params C.zyphrax_params_t
	params.level = 3
	params.block_size = 65536
	params.checksum = 0

	res := C.zyphrax_compress(cSrc, cLen, cDst, bound, &params)

	if res == 0 {
		return nil, errors.New("compression failed")
	}

	return dst[:res], nil
}

func Decompress(data []byte, cap int) ([]byte, error) {
	if len(data) == 0 {
		return nil, nil
	}
	if cap == 0 {
		cap = len(data) * 10
	}

	cSrc := (*C.uchar)(unsafe.Pointer(&data[0]))
	cLen := C.size_t(len(data))

	dst := make([]byte, cap)
	cDst := (*C.uchar)(unsafe.Pointer(&dst[0]))

	res := C.zyphrax_decompress(cSrc, cLen, cDst, C.size_t(cap))

	if res == 0 {
		return nil, errors.New("decompression failed")
	}

	return dst[:res], nil
}
