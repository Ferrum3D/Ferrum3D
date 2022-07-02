using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2F : IEquatable<Vector2F>
    {
        public static Vector2F Zero => new(0);
        public static Vector2F One => new(1);

        public static Vector2F UnitX => new(1, 0);
        public static Vector2F UnitY => new(0, 1);

        public float LengthSq
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => Dot(this, this);
        }

        public Vector2F Normalized
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
        public Vector2F(float value) : this(value, value)
        {
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector2F(float x, float y)
        {
            X = x;
            Y = y;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Dot(Vector2F other)
        {
            return Dot(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Dot(Vector2F lhs, Vector2F rhs)
        {
            return lhs.X * rhs.X + lhs.Y * rhs.Y;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector2F LerpClamped(Vector2F dst, float f)
        {
            return LerpClamped(this, dst, f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector2F Lerp(Vector2F dst, float f)
        {
            return Lerp(this, dst, f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F LerpClamped(Vector2F src, Vector2F dst, float f)
        {
            return Lerp(src, dst, MathF.Saturate(f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F Lerp(Vector2F src, Vector2F dst, float f)
        {
            return (dst - src) * f + src;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F MulEach(Vector2F lhs, Vector2F rhs)
        {
            return new Vector2F(lhs.X * rhs.X, lhs.Y * rhs.Y);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(Vector2F lhs, Vector2F rhs, float epsilon = MathF.Epsilon)
        {
            return MathF.AreApproxEqual(lhs.X, rhs.X)
                   && MathF.AreApproxEqual(lhs.Y, rhs.Y);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F operator -(Vector2F vector)
        {
            return new Vector2F(-vector.X, -vector.Y);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F operator +(Vector2F lhs, Vector2F rhs)
        {
            return new Vector2F(lhs.X + rhs.X, lhs.Y + rhs.Y);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F operator -(Vector2F lhs, Vector2F rhs)
        {
            return new Vector2F(lhs.X - rhs.X, lhs.Y - rhs.Y);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F operator *(Vector2F vector, float f)
        {
            return new Vector2F(vector.X * f, vector.Y * f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector2F operator /(Vector2F vector, float f)
        {
            return vector * (1f / f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override string ToString()
        {
            return $"({X}; {Y})";
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Vector2F other)
        {
            return AreApproxEqual(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object obj)
        {
            return obj is Vector2F other && Equals(other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override int GetHashCode()
        {
            unchecked
            {
                return (X.GetHashCode() * 397) ^ Y.GetHashCode();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Vector2F left, Vector2F right)
        {
            return left.Equals(right);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Vector2F left, Vector2F right)
        {
            return !left.Equals(right);
        }
    }
}
