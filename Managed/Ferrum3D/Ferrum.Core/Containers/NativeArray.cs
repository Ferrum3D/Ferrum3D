using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Containers
{
    [StructLayout(LayoutKind.Sequential)]
    public struct NativeArray<T> : IReadOnlyList<T>, IDisposable
        where T : unmanaged
    {
        public static readonly NativeArray<T> Empty;
        public int Count => (int)GetSize();
        public long LongCount => GetSize();

        public IntPtr DataPointer
        {
            get
            {
                unsafe
                {
                    return new IntPtr(Handle.Begin);
                }
            }
        }

        public T this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ElementAt(index);

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetElementAt(index, ref value);
        }

        internal ByteBuffer.Native Handle;

        private static readonly int elementSize = Marshal.SizeOf<T>();

        public NativeArray(long size)
        {
            Handle = new ByteBuffer.Native();
            ByteBuffer.ConstructNative((ulong)(size * elementSize), ref Handle);
        }

        public NativeArray(int size) : this((long)size)
        {
        }

        public NativeArray(IReadOnlyList<T> collection) : this(collection.Count)
        {
            for (var i = 0; i < collection.Count; ++i)
            {
                var elem = collection[i];
                SetElementAtUnchecked(i, ref elem);
            }
        }

        public static NativeArray<T> FromHandle(in ByteBuffer.Native handle)
        {
            var result = new NativeArray<T>();
            result.Handle = handle;
            return result;
        }

        public static NativeArray<IntPtr> FromObjectCollection<TObject>(IEnumerable<TObject> collection)
            where TObject : UnmanagedObject
        {
            return collection != null
                ? new NativeArray<IntPtr>(collection.Select(x => x.Handle).ToArray())
                : NativeArray<IntPtr>.Empty;
        }

        public IEnumerator<T> GetEnumerator()
        {
            for (var i = 0; i < Count; ++i)
            {
                yield return ElementAtUnchecked(i);
            }
        }

        public ByteBuffer.Native Detach()
        {
            var handle = Handle;
            Handle = new ByteBuffer.Native();
            return handle;
        }

        public void Dispose()
        {
            ByteBuffer.DestructNative(ref Handle);
        }

        private long GetSize()
        {
            unsafe
            {
                return (T*)Handle.End - (T*)Handle.Begin;
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
