using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Containers
{
    public sealed class NativeArray<T> : ByteBuffer, IReadOnlyList<T>
        where T : unmanaged
    {

        public int Count => (int)SizeNative(Handle);
        public long LongCount => (long)SizeNative(Handle);

        public T this[int index] => ElementAt(index);

        private static readonly int elementSize = Marshal.SizeOf<T>();

        public NativeArray(ulong size) : base(size * (ulong)elementSize)
        {
        }

        public NativeArray(int size) : base(size * elementSize)
        {
        }

        public NativeArray(IReadOnlyList<T> collection) : base(collection.Count * elementSize)
        {
            var ptr = DataPointer;
            for (var i = 0; i < collection.Count; ++i)
            {
                unsafe
                {
                    var elemPtr = (T*)(ptr + i * elementSize);
                    *elemPtr = collection[i];
                }
            }
        }

        private NativeArray(IntPtr handle) : base(handle)
        {
        }

        public static NativeArray<IntPtr> FromObjectCollection<TObject>(IEnumerable<TObject> collection)
            where TObject : UnmanagedObject
        {
            return collection != null
                ? new NativeArray<IntPtr>(collection.Select(x => x.Handle).ToArray())
                : null;
        }

        public static NativeArray<T> FromHandle(IntPtr handle)
        {
            return new NativeArray<T>(handle);
        }

        public IEnumerator<T> GetEnumerator()
        {
            for (var i = 0; i < Count; ++i)
            {
                yield return ElementAt(i);
            }
        }

        private T ElementAt(int index)
        {
            if (DataPointer == IntPtr.Zero || index < 0 || index > Count)
            {
                throw new IndexOutOfRangeException();
            }

            unsafe
            {
                return ((T*)DataPointer)![index];
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
