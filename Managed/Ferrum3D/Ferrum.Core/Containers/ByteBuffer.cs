using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Containers
{
    public class ByteBuffer : UnmanagedObject, IEnumerable<byte>
    {
        public int Size => (int)SizeNative(Handle);
        public long LongSize => (long)SizeNative(Handle);

        public byte this[int index] => ByteAt(index, DataNative(Handle));

        public IntPtr DataPointer => DataNative(Handle);

        public ByteBuffer(IntPtr handle) : base(handle)
        {
        }

        public ByteBuffer(ulong size) : this(ConstructNative(size))
        {
        }

        public ByteBuffer(int size) : this(ConstructNative((ulong)size))
        {
        }

        public static ByteBuffer FromCollection<T>(IReadOnlyList<T> collection)
            where T : unmanaged
        {
            var buffer = new ByteBuffer(collection.Count * Marshal.SizeOf<T>());
            var ptr = buffer.DataPointer;
            for (var i = 0; i < collection.Count; ++i)
            {
                unsafe
                {
                    var elemPtr = (T*)(ptr + i * Marshal.SizeOf<T>());
                    *elemPtr = collection[i];
                }
            }

            return buffer;
        }

        public IEnumerator<byte> GetEnumerator()
        {
            var data = DataNative(Handle);
            for (var i = 0; i < Size; ++i)
            {
                yield return ByteAt(i, data);
            }
        }

        private static byte ByteAt(int index, IntPtr data)
        {
            unsafe
            {
                var ptr = (byte*)data + index;
                return *ptr;
            }
        }

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Construct")]
        private static extern IntPtr ConstructNative(ulong size);

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Size")]
        private static extern ulong SizeNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Data")]
        private static extern IntPtr DataNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
