using System.Runtime.InteropServices;

namespace Ferrum.Core.Utils
{
    public static class NativeUtils
    {
        public static int SizeOf<T>()
            where T : unmanaged
        {
            unsafe
            {
                return sizeof(T);
            }
        }

        public static uint USizeOf<T>()
            where T : unmanaged
        {
            unsafe
            {
                return (uint)sizeof(T);
            }
        }

        public static uint AlignOf<T>()
            where T : unmanaged
        {
            return USizeOf<AlignHelper<T>>() - USizeOf<T>();
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct AlignHelper<T>
            where T : unmanaged
        {
            private readonly byte padding;
            private readonly T value;
        }
    }
}
