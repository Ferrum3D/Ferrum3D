using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Ferrum.Core.Utils
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Uuid : IEquatable<Uuid>
    {
        public static Uuid Empty { get; } = new();

        private unsafe fixed long data[2];

        private static readonly int stringLength = Guid.Empty.ToString().Length;

        public static Uuid Parse(string s)
        {
            FromStringNative(s, out var result);
            return result;
        }

        public static Uuid FromGuid(in Guid guid)
        {
            FromGuidNative(in guid, out var result);
            return result;
        }

        public override string ToString()
        {
            unsafe
            {
                var buffer = stackalloc byte[stringLength];
                ToStringNative(in this, new IntPtr(buffer));
                return Encoding.ASCII.GetString(buffer, stringLength);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Uuid other)
        {
            unsafe
            {
                return data[0] == other.data[0] && data[1] == other.data[1];
            }
        }

        public override bool Equals(object obj)
        {
            return obj is Uuid u && Equals(u) ||
                   obj is Guid g && Equals(FromGuid(g));
        }

        public override int GetHashCode()
        {
            return GetHashNative(in this).GetHashCode();
        }

        public static bool operator ==(Uuid left, Guid right)
        {
            return left == FromGuid(right);
        }

        public static bool operator !=(Uuid left, Guid right)
        {
            return left != FromGuid(right);
        }

        public static bool operator ==(Uuid left, Uuid right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(Uuid left, Uuid right)
        {
            return !left.Equals(right);
        }

        [DllImport("FeCoreBindings", EntryPoint = "UUID_FromGUID")]
        private static extern void FromGuidNative(in Guid guid, out Uuid result);

        [DllImport("FeCoreBindings", EntryPoint = "UUID_FromString", CharSet = CharSet.Ansi)]
        private static extern void FromStringNative(string s, out Uuid result);

        [DllImport("FeCoreBindings", EntryPoint = "UUID_ToString")]
        private static extern void ToStringNative(in Uuid self, IntPtr result);

        [DllImport("FeCoreBindings", EntryPoint = "UUID_GetHash")]
        private static extern ulong GetHashNative(in Uuid self);
    }
}
