import ctypes
import os
import sys

# Load Library
# We try to find libzyphrax.dylib (mac) or .so (linux)
# Assumes it is in the root directory relative to this script?
# Or in current working directory.

def _load_lib():
    lib_name = "libzyphrax.dylib" if sys.platform == "darwin" else "libzyphrax.so"
    # Try looking in CWD or parent
    paths = [
        os.path.join(os.getcwd(), lib_name),
        os.path.join(os.path.dirname(__file__), "..", "..", lib_name),
        os.path.join(os.path.dirname(__file__), lib_name),
        lib_name
    ]
    
    for p in paths:
        if os.path.exists(p):
            try:
                return ctypes.CDLL(p)
            except OSError:
                continue
    # One last try system path
    try:
        return ctypes.CDLL(lib_name)
    except OSError:
        return None

_lib = _load_lib()

if not _lib:
    raise RuntimeError("Could not load libzyphrax")

# Types
class ZyphraxParams(ctypes.Structure):
    _fields_ = [
        ("level", ctypes.c_uint32),
        ("block_size", ctypes.c_uint32),
        ("checksum", ctypes.c_uint32),
    ]

# Signatures
_lib.zyphrax_compress_bound.argtypes = [ctypes.c_size_t]
_lib.zyphrax_compress_bound.restype = ctypes.c_size_t

_lib.zyphrax_compress.argtypes = [
    ctypes.POINTER(ctypes.c_uint8), # src
    ctypes.c_size_t,                # src_size
    ctypes.POINTER(ctypes.c_uint8), # dst
    ctypes.c_size_t,                # dst_cap
    ctypes.POINTER(ZyphraxParams)   # params
]
_lib.zyphrax_compress.restype = ctypes.c_size_t

_lib.zyphrax_decompress.argtypes = [
    ctypes.POINTER(ctypes.c_uint8), # src
    ctypes.c_size_t,                # src_size
    ctypes.POINTER(ctypes.c_uint8), # dst
    ctypes.c_size_t                 # dst_cap
]
_lib.zyphrax_decompress.restype = ctypes.c_size_t

# Wrapper
def compress(data: bytes, level=3, block_size=65536) -> bytes:
    src_len = len(data)
    bound = _lib.zyphrax_compress_bound(src_len)
    
    # Alloc dst
    out_buf = ctypes.create_string_buffer(bound)
    
    params = ZyphraxParams(level=level, block_size=block_size, checksum=0)
    
    # Cast pointers
    src_ptr = (ctypes.c_uint8 * src_len).from_buffer_copy(data)
    dst_ptr = ctypes.cast(out_buf, ctypes.POINTER(ctypes.c_uint8))
    
    encoded_size = _lib.zyphrax_compress(
        src_ptr, src_len,
        dst_ptr, bound,
        ctypes.byref(params)
    )
    
    if encoded_size == 0 and src_len > 0:
        raise RuntimeError("Compression failed")
        
    return out_buf.raw[:encoded_size]

def decompress(data: bytes, dst_cap: int = 0) -> bytes:
    if dst_cap == 0:
        # Estimation if not provided (safe bet usually)
        dst_cap = len(data) * 10 
        
    src_len = len(data)
    out_buf = ctypes.create_string_buffer(dst_cap)
    
    src_ptr = (ctypes.c_uint8 * src_len).from_buffer_copy(data)
    dst_ptr = ctypes.cast(out_buf, ctypes.POINTER(ctypes.c_uint8))
    
    dec_size = _lib.zyphrax_decompress(src_ptr, src_len, dst_ptr, dst_cap)
    
    if dec_size == 0:
        raise RuntimeError("Decompression failed")
        
    return out_buf.raw[:dec_size]
