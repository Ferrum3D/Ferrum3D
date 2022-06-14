using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4F : IEquatable<Vector4F>
    {
        public static Vector4F Zero => new(0);
        public static Vector4F One => new(1);

        public static Vector4F UnitX => new(1, 0, 0, 0);
        public static Vector4F UnitY => new(0, 1, 0, 0);
        public static Vector4F UnitZ => new(0, 0, 1, 0);
        public static Vector4F UnitW => new(0, 0, 0, 1);

        public float LengthSq
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => Dot(this, this);
        }

        public Vector4F Normalized
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
        public float W;

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

        public float Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MathF.Sqrt(LengthSq);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => this *= value / Length;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F(Vector3F xyz, float w = 1f) : this(xyz.X, xyz.Y, xyz.Z, w)
        {
        }

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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3F GetVector3()
        {
            return new Vector3F(X, Y, Z);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Dot(Vector4F other)
        {
            return Dot(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Dot(Vector4F lhs, Vector4F rhs)
        {
            return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z + lhs.W * rhs.W;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F LerpClamped(Vector4F dst, float f)
        {
            return LerpClamped(this, dst, f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F Lerp(Vector4F dst, float f)
        {
            return Lerp(this, dst, f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F LerpClamped(Vector4F src, Vector4F dst, float f)
        {
            return Lerp(src, dst, MathF.Saturate(f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F Lerp(Vector4F src, Vector4F dst, float f)
        {
            return (dst - src) * f + src;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F MulEach(Vector4F lhs, Vector4F rhs)
        {
            return new Vector4F(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z, lhs.W * rhs.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(Vector4F lhs, Vector4F rhs, float epsilon = MathF.Epsilon)
        {
            return MathF.AreApproxEqual(lhs.X, rhs.X, epsilon)
                   && MathF.AreApproxEqual(lhs.Y, rhs.Y, epsilon)
                   && MathF.AreApproxEqual(lhs.Z, rhs.Z, epsilon)
                   && MathF.AreApproxEqual(lhs.W, rhs.W, epsilon);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F operator -(Vector4F vector)
        {
            return new Vector4F(-vector.X, -vector.Y, -vector.Z, -vector.W);
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
        public static Vector4F operator *(Vector4F vector, float f)
        {
            return new Vector4F(vector.X * f, vector.Y * f, vector.Z * f, vector.W * f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F operator /(Vector4F vector, float f)
        {
            return vector * (1f / f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Vector4F other)
        {
            return AreApproxEqual(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object obj)
        {
            return obj is Vector4F other && Equals(other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                hashCode = (hashCode * 397) ^ W.GetHashCode();
                return hashCode;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override string ToString()
        {
            return $"({X}; {Y}; {Z}; {W})";
        }
    }
}
