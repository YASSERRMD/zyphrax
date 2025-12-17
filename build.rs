fn main() {
    println!("cargo:rerun-if-changed=src/zyphrax.c");
    println!("cargo:rerun-if-changed=src/zyphrax.h");
    
    // Compile C kernel
    let mut build = cc::Build::new();
    
    build
        .file("src/zyphrax.c")
        .file("src/zyphrax_block.c")
        .file("src/zyphrax_lz77.c")
        .file("src/zyphrax_simd.c")
        .file("src/zyphrax_seq.c")
        .file("src/zyphrax_huff.c")
        .include("src")
        .flag_if_supported("-O3");

    // Add architecture specific flags
    #[cfg(target_arch = "x86_64")]
    {
        build.flag_if_supported("-mavx2");
        build.flag_if_supported("-mbmi2");
    }
    
    // For ARM (Neon is usually valid by default on aarch64, but we can add native)
    #[cfg(target_arch = "aarch64")]
    {
        // build.flag_if_supported("-march=native"); // Safe for local build, maybe not for distribution?
        // Using neon implicit in aarch64
    }

    build.compile("zyphrax");
}
