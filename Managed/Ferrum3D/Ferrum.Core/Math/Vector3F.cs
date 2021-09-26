using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3F : IEquatable<Vector3F>
    {
        public static Vector3F Zero => new Vector3F(0);
        public static Vector3F One => new Vector3F(1);

        public static Vector3F UnitX => new Vector3F(1, 0, 0);
        public static Vector3F UnitY => new Vector3F(0, 1, 0);
        public static Vector3F UnitZ => new Vector3F(0, 0, 1);

        public float LengthSq
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => Dot(this, this);
        }

        public Vector3F Normalized
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                var result = this;
                result.Length = 1;
                return result;
            }
        }

        public float X;
        public float Y;
        public float Z;

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
                    default:
                        throw new IndexOutOfRangeException(nameof(index));
                }
            }
        }

        public float Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MathF.Sqrt(LengthSq);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => this *= value / Length;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3F(float value) : this(value, value, value)
        {
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3F(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Dot(Vector3F other)
        {
            return Dot(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Dot(Vector3F lhs, Vector3F rhs)
        {
            return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3F LerpClamped(Vector3F dst, float f)
        {
            return LerpClamped(this, dst, f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3F Lerp(Vector3F dst, float f)
        {
            return Lerp(this, dst, f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F LerpClamped(Vector3F src, Vector3F dst, float f)
        {
            return Lerp(src, dst, MathF.Saturate(f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F Lerp(Vector3F src, Vector3F dst, float f)
        {
            return (dst - src) * f + src;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F MulEach(Vector3F lhs, Vector3F rhs)
        {
            return new Vector3F(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(Vector3F lhs, Vector3F rhs, float epsilon = MathF.Epsilon)
        {
            return MathF.AreApproxEqual(lhs.X, rhs.X)
                   && MathF.AreApproxEqual(lhs.Y, rhs.Y)
                   && MathF.AreApproxEqual(lhs.Z, rhs.Z);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F operator -(Vector3F vector)
        {
            return new Vector3F(-vector.X, -vector.Y, -vector.Z);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F operator +(Vector3F lhs, Vector3F rhs)
        {
            return new Vector3F(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F operator -(Vector3F lhs, Vector3F rhs)
        {
            return new Vector3F(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F operator *(Vector3F vector, float f)
        {
            return new Vector3F(vector.X * f, vector.Y * f, vector.Z * f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector3F operator /(Vector3F vector, float f)
        {
            return vector * (1f / f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override string ToString()
        {
            return $"({X}; {Y}; {Z})";
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Vector3F other)
        {
            return AreApproxEqual(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object obj)
        {
            return obj is Vector3F other && Equals(other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                return hashCode;
            }
        }
    }
}
