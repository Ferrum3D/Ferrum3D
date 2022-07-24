using System.Runtime.InteropServices;
using Ferrum.Core.Entities;
using Ferrum.Core.Math;

namespace Ferrum.Core.Components
{
    [StructLayout(LayoutKind.Sequential)]
    [Component("79E8B950-E483-45F7-B08B-07DABC47D3DA", Alignment = 16, Unmanaged = true)]
    public readonly struct LocalToWorldComponent
    {
        public readonly Matrix4x4F Matrix;

        public LocalToWorldComponent(in Matrix4x4F matrix)
        {
            Matrix = matrix;
        }

        public static bool operator ==(in LocalToWorldComponent left, in LocalToWorldComponent right)
        {
            return left.Equals(in right);
        }

        public static bool operator !=(in LocalToWorldComponent left, in LocalToWorldComponent right)
        {
            return !left.Equals(in right);
        }

        public bool Equals(in LocalToWorldComponent other)
        {
            return Matrix4x4F.AreApproxEqual(Matrix, other.Matrix);
        }

        public override bool Equals(object obj)
        {
            return obj is LocalToWorldComponent other && Equals(other);
        }

        public override int GetHashCode()
        {
            return Matrix.GetHashCode();
        }
    }
}
