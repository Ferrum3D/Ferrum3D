using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Containers
{
    public sealed class NativeArray<T> : ByteBuffer, IReadOnlyList<T>
        where T : unmanaged
    {
        public int Count => (int)SizeNative(Handle) / elementSize;
        public long LongCount => (long)SizeNative(Handle) / elementSize;

        public T this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ElementAt(index);

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetElementAt(index, ref value);
        }

        private static readonly int elementSize = Marshal.SizeOf<T>();

        public NativeArray(long size) : base(size * elementSize)
        {
        }

        public NativeArray(int size) : base(size * elementSize)
        {
        }

        public NativeArray(IReadOnlyList<T> collection) : base(collection.Count * elementSize)
        {
            for (var i = 0; i < collection.Count; ++i)
            {
                var elem = collection[i];
                SetElementAtUnchecked(i, ref elem);
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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static NativeArray<T> FromHandle(IntPtr handle)
        {
            return new NativeArray<T>(handle);
        }

        public IEnumerator<T> GetEnumerator()
        {
            for (var i = 0; i < Count; ++i)
            {
                yield return ElementAtUnchecked(i);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private T ElementAtUnchecked(long index)
        {
            unsafe
            {
                return ((T*)DataPointer)![index];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void SetElementAtUnchecked(long index, ref T item)
        {
            unsafe
            {
                ((T*)DataPointer)![index] = item;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private T ElementAt(long index)
        {
            if (DataPointer == IntPtr.Zero || index < 0 || index > Count)
            {
                throw new IndexOutOfRangeException();
            }

            return ElementAtUnchecked(index);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void SetElementAt(long index, ref T item)
        {
            if (DataPointer == IntPtr.Zero || index < 0 || index > Count)
            {
                throw new IndexOutOfRangeException();
            }

            SetElementAtUnchecked(index, ref item);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
