using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Containers
{
    public sealed class NativeList<T> : ByteBuffer, IList<T>
        where T : unmanaged
    {
        public int Count => (int)LongCount;

        public long LongCount { get; private set; } = 0;

        public bool IsReadOnly => false;

        private static readonly int elementSize = Marshal.SizeOf<T>();

        public int Capacity
        {
            get => (int)LongCapacity;
            set => LongCapacity = value;
        }

        public long LongCapacity
        {
            get => Handle == IntPtr.Zero ? 0 : (long)SizeNative(Handle) / elementSize;
            set
            {
                if (value < Count)
                {
                    throw new IndexOutOfRangeException();
                }

                if (value * elementSize == (long)SizeNative(Handle))
                {
                    return;
                }

                if (value <= 0)
                {
                    DestructNative(Handle);
                    Handle = IntPtr.Zero;
                    return;
                }

                var newData = new NativeArray<T>(value);
                CopyToNative(Handle, newData.Handle);
                if (Handle != IntPtr.Zero)
                {
                    DestructNative(Handle);
                }

                Handle = newData.Detach();
            }
        }

        public T this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => ElementAt(index);

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetElementAt(index, ref value);
        }

        public NativeList() : base(IntPtr.Zero)
        {
        }

        public NativeList(IEnumerable<T> collection) : base(IntPtr.Zero)
        {
            if (collection is IReadOnlyList<T> c)
            {
                LongCapacity = c.Count;
                LongCount = c.Count;
                
                for (var i = 0; i < c.Count; ++i)
                {
                    var elem = c[i];
                    SetElementAtUnchecked(i, ref elem);
                }
                return;
            }

            foreach (var item in collection)
            {
                Add(item);
            }
        }

        public NativeList(long capacity) : base(capacity)
        {
        }

        public NativeList(int capacity) : base(capacity)
        {
        }

        public IEnumerator<T> GetEnumerator()
        {
            for (var i = 0; i < LongCount; ++i)
            {
                yield return ElementAtUnchecked(i);
            }
        }

        public void Add(T item)
        {
            EnsureCapacity(LongCount + 1);
            SetElementAtUnchecked(LongCount++, ref item);
        }

        public void Clear()
        {
            LongCount = 0;
        }

        public bool Contains(T item)
        {
            return IndexOf(item) != -1;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            if (array.Rank != 1)
            {
                throw new ArgumentException("Multidimensional arrays are not supported.");
            }

            if (array == null)
            {
                throw new ArgumentNullException(nameof(array));
            }

            if (arrayIndex < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(arrayIndex));
            }

            if (array.Length - arrayIndex < Count)
            {
                throw new ArgumentException("Not enough elements after arrayIndex in the destination array.");
            }

            for (var i = 0; i < Count; ++i)
            {
                array[i + arrayIndex] = this[i];
            }
        }

        public bool Remove(T item)
        {
            var found = false;
            for (var i = 0; i < LongCount; ++i)
            {
                if (found)
                {
                    var elem = ElementAtUnchecked(i);
                    SetElementAtUnchecked(i - 1, ref elem);
                    continue;
                }

                if (ElementAtUnchecked(i).Equals(item))
                {
                    found = true;
                }
            }

            if (found)
            {
                LongCount--;
            }

            return found;
        }

        public int IndexOf(T item)
        {
            for (var i = 0; i < Count; ++i)
            {
                if (item.Equals(this[i]))
                {
                    return i;
                }
            }

            return -1;
        }

        public void Insert(int index, T item)
        {
            EnsureCapacity(++LongCount);
            for (var i = index; i < LongCount - 1; ++i)
            {
                var elem = ElementAtUnchecked(i);
                SetElementAtUnchecked(i + 1, ref elem);
            }
        }

        public void RemoveAt(int index)
        {
            LongCount--;
            for (var i = index + 1; i < LongCount + 1; ++i)
            {
                var elem = ElementAtUnchecked(i);
                SetElementAtUnchecked(i - 1, ref elem);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void EnsureCapacity(long min)
        {
            if (LongCapacity >= min)
            {
                return;
            }

            var capacity = LongCapacity == 0 ? 4 : LongCapacity * 2;
            if (capacity < min)
            {
                capacity = min;
            }

            LongCapacity = capacity;
        }
    }
}
