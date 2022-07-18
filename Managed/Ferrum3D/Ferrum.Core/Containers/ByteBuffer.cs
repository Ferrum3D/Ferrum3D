using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Containers
{
    public static class ByteBuffer
    {
        [DllImport("FeCoreBindings", EntryPoint = "ByteBuffer_Construct")]
        internal static extern void ConstructNative(ulong size, ref Native result);

        [DllImport("FeCoreBindings", EntryPoint = "ByteBuffer_CopyTo")]
        internal static extern void CopyToNative(ref Native self, ref Native dest);

        [DllImport("FeCoreBindings", EntryPoint = "ByteBuffer_Destruct")]
        internal static extern void DestructNative(ref Native self);

        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct Native
        {
            public byte* Begin;
            public byte* End;
        }
    }
}
