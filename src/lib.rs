use libc::{size_t, c_uint};
use std::ptr;

// Raw FFI
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct ZyphraxParams {
    pub level: u32,
    pub block_size: u32,
    pub checksum: u32,
}

impl Default for ZyphraxParams {
    fn default() -> Self {
        Self {
            level: 3,
            block_size: 64 * 1024,
            checksum: 0,
        }
    }
}

extern "C" {
    fn zyphrax_compress_bound(src_size: size_t) -> size_t;
    fn zyphrax_compress(
        src: *const u8,
        src_size: size_t,
        dst: *mut u8,
        dst_cap: size_t,
        params: *const ZyphraxParams,
    ) -> size_t;
    fn zyphrax_decompress(
        src: *const u8,
        src_size: size_t,
        dst: *mut u8,
        dst_cap: size_t,
    ) -> size_t;
}

// Safe Rust API
pub fn compress(data: &[u8], params: Option<ZyphraxParams>) -> Vec<u8> {
    let params = params.unwrap_or_default();
    let bound = unsafe { zyphrax_compress_bound(data.len()) };
    let mut out = vec![0u8; bound];
    
    let compressed_size = unsafe {
        zyphrax_compress(
            data.as_ptr(),
            data.len(),
            out.as_mut_ptr(),
            out.len(),
            &params,
        )
    };
    
    if compressed_size == 0 {
        // Should handle error gracefully, but for simplicity of vector return:
        return Vec::new(); // Error or empty
    }
    
    out.truncate(compressed_size);
    out
}

pub fn decompress(data: &[u8], original_size_guess: usize) -> Option<Vec<u8>> {
    // Phase 9 pending: Decompression currently returns 0 always.
    // We allocate guess.
    let mut out = vec![0u8; original_size_guess];
    
    let res = unsafe {
        zyphrax_decompress(
            data.as_ptr(),
            data.len(),
            out.as_mut_ptr(),
            out.len(),
        )
    };
    
    if res == 0 {
        return None;
    }
    
    out.truncate(res);
    Some(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_params_layout() {
        assert_eq!(std::mem::size_of::<ZyphraxParams>(), 12);
    }

    #[test]
    fn test_compress_basic() {
        let data = b"hello hello hello hello";
        let compressed = compress(data, None);
        assert!(!compressed.is_empty());
        assert!(compressed.len() > 12); // Headers
        // assert!(compressed.len() < data.len()); // Small data might expand slightly due to headers + block overhead
    }
    
    #[test]
    fn test_compress_large() {
        let data = vec![b'A'; 1024 * 1024];
        let compressed = compress(&data, None);
        // Should compress well
        let ratio = compressed.len() as f64 / data.len() as f64;
        println!("Ratio: {:.2}%", ratio * 100.0);
        assert!(ratio < 0.1); 
    }
}
