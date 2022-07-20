using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Entities
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ComponentType
    {
        public readonly Guid Type;
        public readonly uint Alignment;
        public readonly uint DataSize;

        private ComponentType(Guid type, uint alignment, uint dataSize)
        {
            Type = type;
            Alignment = alignment;
            DataSize = dataSize;
        }

        public static ComponentType Create<T>(in Guid type, uint alignment = 16)
            where T : unmanaged
        {
            return new ComponentType(type, alignment, (uint)Marshal.SizeOf<T>());
        }
    }
}
