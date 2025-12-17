using System;
using System.Runtime.InteropServices;

namespace Zyphrax
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ZyphraxParams
    {
        public uint Level;
        public uint BlockSize;
        public uint Checksum;
    }

    public static class Compressor
    {
        const string LibName = "libzyphrax"; // Will look for libzyphrax.dylib, .so, .dll

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr zyphrax_compress_bound(IntPtr srcSize);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr zyphrax_compress(
            IntPtr src,
            IntPtr srcSize,
            IntPtr dst,
            IntPtr dstCap,
            ref ZyphraxParams par
        );

        public static byte[] Compress(byte[] data)
        {
            if (data == null || data.Length == 0) return new byte[0];

            IntPtr bound = zyphrax_compress_bound((IntPtr)data.Length);
            byte[] buffer = new byte[(int)bound];

            ZyphraxParams p = new ZyphraxParams
            {
                Level = 3,
                BlockSize = 65536,
                Checksum = 0
            };

            GCHandle hSrc = GCHandle.Alloc(data, GCHandleType.Pinned);
            GCHandle hDst = GCHandle.Alloc(buffer, GCHandleType.Pinned);

            try
            {
                IntPtr res = zyphrax_compress(
                    hSrc.AddrOfPinnedObject(),
                    (IntPtr)data.Length,
                    hDst.AddrOfPinnedObject(),
                    (IntPtr)buffer.Length,
                    ref p
                );

                int size = (int)res;
                if (size == 0) throw new Exception("Compression failed");

                byte[] ret = new byte[size];
                Array.Copy(buffer, ret, size);
                return ret;
            }
            finally
            {
                hSrc.Free();
                hDst.Free();
            }
        }
    }
}
