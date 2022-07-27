using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Entities
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ArchetypeChunk
    {
        public ulong LongCount => CountNative(handle);
        public int Count => (int)CountNative(handle);
        private readonly IntPtr handle;

        internal ArchetypeChunk(IntPtr handle)
        {
            this.handle = handle;
        }

        public unsafe T* OffsetOf<T>()
            where T : unmanaged
        {
            return (T*)OffsetOf(ComponentInfo<T>.Type);
        }

        public IntPtr OffsetOf(in Uuid typeId)
        {
            return OffsetOfNative(handle, in typeId);
        }

        [DllImport("FeCoreBindings", EntryPoint = "ArchetypeChunk_OffsetOf")]
        private static extern IntPtr OffsetOfNative(IntPtr self, in Uuid typeId);

        [DllImport("FeCoreBindings", EntryPoint = "ArchetypeChunk_Count")]
        private static extern ulong CountNative(IntPtr self);
    }
}
