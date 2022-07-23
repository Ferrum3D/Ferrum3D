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

        public static ComponentType[] CreateList<T>()
            where T : unmanaged
        {
            return new[] { ComponentInfo<T>.ComponentType };
        }

        public static ComponentType[] CreateList<T1, T2>()
            where T1 : unmanaged
            where T2 : unmanaged
        {
            return new[] { ComponentInfo<T1>.ComponentType, ComponentInfo<T2>.ComponentType };
        }

        public static ComponentType[] CreateList<T1, T2, T3>()
            where T1 : unmanaged
            where T2 : unmanaged
            where T3 : unmanaged
        {
            return new[] { ComponentInfo<T1>.ComponentType, ComponentInfo<T2>.ComponentType, ComponentInfo<T3>.ComponentType };
        }

        public static ComponentType[] CreateList<T1, T2, T3, T4>()
            where T1 : unmanaged
            where T2 : unmanaged
            where T3 : unmanaged
            where T4 : unmanaged
        {
            return new[]
            {
                ComponentInfo<T1>.ComponentType, ComponentInfo<T2>.ComponentType, ComponentInfo<T3>.ComponentType,
                ComponentInfo<T4>.ComponentType
            };
        }
    }
}
