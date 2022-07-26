using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public partial struct Quaternion : IEquatable<Quaternion>
    {
        public float LengthSq
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => Dot(this, this);
        }

        public Quaternion Normalized
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                var result = this;
                result.Length = 1;
                return result;
            }
        }

        public Vector3F Euler
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => FromEulerAnglesNative(value.X, value.Y, value.Z, out this);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                unsafe
                {
                    var result = new Vector3F();
                    GetEulerAnglesNative(in this, new IntPtr(&result));
                    return result;
                }
            }
        }

        public Quaternion Conjugated => new(-X, -Y, -Z, W);

        public Quaternion Inverse => Conjugated / LengthSq;

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
        public Quaternion(Vector3F xyz, float w = 1f) : this(xyz.X, xyz.Y, xyz.Z, w)
        {
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Quaternion(float value) : this(value, value, value, value)
        {
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Quaternion(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion CreateRotationX(float angle)
        {
            CreateRotationXNative(angle, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion CreateRotationY(float angle)
        {
            CreateRotationYNative(angle, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion CreateRotationZ(float angle)
        {
            CreateRotationZNative(angle, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion FromAxisAngle(Vector3F axis, float angle)
        {
            FromAxisAngleNative(axis.X, axis.Y, axis.Z, angle, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion FromEulerAngles(Vector3F angles)
        {
            return new Quaternion
            {
                Euler = angles
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion FromEulerAngles(float x, float y, float z)
        {
            return new Quaternion
            {
                Euler = new Vector3F(x, y, z)
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void GetAxisAngle(out Vector3F axis, out float angle)
        {
            unsafe
            {
                fixed (Vector3F* ptr = &axis)
                {
                    GetAxisAngleNative(this, new IntPtr(ptr), out angle);
                }
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Dot(Quaternion other)
        {
            return Dot(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Dot(Quaternion lhs, Quaternion rhs)
        {
            return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z + lhs.W * rhs.W;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion Lerp(Quaternion src, Quaternion dst, float f)
        {
            LerpNative(in src, in dst, f, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion SLerp(Quaternion src, Quaternion dst, float f)
        {
            SLerpNative(in src, in dst, f, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(Quaternion lhs, Quaternion rhs, float epsilon = MathF.Epsilon)
        {
            return MathF.AreApproxEqual(lhs.X, rhs.X, epsilon)
                   && MathF.AreApproxEqual(lhs.Y, rhs.Y, epsilon)
                   && MathF.AreApproxEqual(lhs.Z, rhs.Z, epsilon)
                   && MathF.AreApproxEqual(lhs.W, rhs.W, epsilon);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion operator -(Quaternion vector)
        {
            return new Quaternion(-vector.X, -vector.Y, -vector.Z, -vector.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion operator +(Quaternion lhs, Quaternion rhs)
        {
            return new Quaternion(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion operator -(Quaternion lhs, Quaternion rhs)
        {
            return new Quaternion(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion operator *(Quaternion lhs, Quaternion rhs)
        {
            MultiplyNative(in lhs, in rhs, out var quaternion);
            return quaternion;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion operator *(Quaternion vector, float f)
        {
            return new Quaternion(vector.X * f, vector.Y * f, vector.Z * f, vector.W * f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Quaternion operator /(Quaternion vector, float f)
        {
            return vector * (1f / f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override string ToString()
        {
            return $"({X}; {Y}; {Z}; {W})";
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Quaternion other)
        {
            return AreApproxEqual(this, other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object obj)
        {
            return obj is Quaternion other && Equals(other);
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
        public static bool operator ==(Quaternion left, Quaternion right)
        {
            return left.Equals(right);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Quaternion left, Quaternion right)
        {
            return !left.Equals(right);
        }
    }
}
