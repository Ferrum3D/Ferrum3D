using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4F
    {
        public float X;
        public float Y;
        public float Z;
        public float W;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F(float value) : this(value, value, value, value)
        {
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public float this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                switch (index)
                {
                    case 0:
                        return X;
                    case 1:
                        return Y;
                    case 2:
                        return Z;
                    case 3:
                        return W;
                    default:
                        throw new IndexOutOfRangeException(nameof(index));
                }
            }
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                switch (index)
                {
                    case 0:
                        X = value;
                        break;
                    case 1:
                        Y = value;
                        break;
                    case 2:
                        Z = value;
                        break;
                    case 3:
                        W = value;
                        break;
                    default:
                        throw new IndexOutOfRangeException(nameof(index));
                }
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F operator +(Vector4F lhs, Vector4F rhs)
        {
            return new Vector4F(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F operator -(Vector4F lhs, Vector4F rhs)
        {
            return new Vector4F(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F operator -(Vector4F vector)
        {
            return new Vector4F(-vector.X, -vector.Y, -vector.Z, -vector.W);
        }
    }
}
