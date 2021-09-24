using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Color : IEquatable<Color>
    {
        [FieldOffset(0)] private Vector4F color;
        private const float MaxByte = 255.0f;
        private const float InvMaxByte = 1.0f / 255.0f;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color(Vector4F rgba)
        {
            color = rgba;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color(Vector3F rgb, float a = 1f) : this(new Vector4F(rgb, a))
        {
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color(float r, float g, float b, float a) : this(new Vector4F(r, g, b, a))
        {
        }

        public readonly static Color Zero = new Color(0, 0, 0, 0);

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color FromBytes(byte r, byte g, byte b, byte a)
        {
            var red = r * InvMaxByte;
            var green = g * InvMaxByte;
            var blue = b * InvMaxByte;
            var alpha = a * InvMaxByte;
            return new Color(red, green, blue, alpha);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color FromUInt32(uint rgba)
        {
            var r = (byte)((rgba >> 24) & 0xFF);
            var g = (byte)((rgba >> 16) & 0xFF);
            var b = (byte)((rgba >> 8) & 0xFF);
            var a = (byte)((rgba >> 0) & 0xFF);

            return FromBytes(r, g, b, a);
        }

        public float this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => color[index];
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => color[index] = value;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F GetVector4()
        {
            return color;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector3F GetVector3()
        {
            return color.GetVector3();
        }

        public float R32
        {
            get => color.X;
            set => color.X = value;
        }

        public float G32
        {
            get => color.Y;
            set => color.Y = value;
        }

        public float B32
        {
            get => color.Z;
            set => color.Z = value;
        }

        public float A32
        {
            get => color.W;
            set => color.W = value;
        }

        public byte R8
        {
            get => (byte) (MaxByte * color.X);
            set => color.X = value * InvMaxByte;
        }

        public byte G8
        {
            get => (byte) (MaxByte * color.Y);
            set => color.Y = value * InvMaxByte;
        }

        public byte B8
        {
            get => (byte) (MaxByte * color.Z);
            set => color.Z = value * InvMaxByte;
        }

        public byte A8
        {
            get => (byte) (MaxByte * color.W);
            set => color.W = value * InvMaxByte;
        }
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color ToLinear()
        {
            float ElemToLinear(float v)
            {
                return v <= 0.04045f ? v / 12.92f : MathF.Pow((v + 0.055f) / 1.055f, 2.4f);
            }

            return new Color(ElemToLinear(R32), ElemToLinear(G32), ElemToLinear(B32), ElemToLinear(A32));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color ToSrgb()
        {
            float ElemToSrgb(float v)
            {
                return v <= 0.0031308f ? v * 12.92f : MathF.Pow(v, 1.0f / 2.4f) * 1.055f - 0.055f;
            }

            return new Color(ElemToSrgb(R32), ElemToSrgb(G32), ElemToSrgb(B32), ElemToSrgb(A32));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color ToLinearApprox()
        {
            float ElemToLinear(float v)
            {
                return MathF.Pow(v, 2.2f);
            }

            return new Color(ElemToLinear(R32), ElemToLinear(G32), ElemToLinear(B32), ElemToLinear(A32));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color ToSrgbApprox()
        {
            float ElemToSrgb(float v)
            {
                return MathF.Pow(v, 1f / 2.2f);
            }

            return new Color(ElemToSrgb(R32), ElemToSrgb(G32), ElemToSrgb(B32), ElemToSrgb(A32));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public uint ToUInt32()
        {
            return (uint) (A8 << 24) | (uint) (B8 << 16) | (uint) (G8 << 8) | R8;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color LerpClamped(Color dst, float f)
        {
            return new Color(color.LerpClamped(dst.color, f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Color Lerp(Color dst, float f)
        {
            return new Color(color.Lerp(dst.color, f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color LerpClamped(Color src, Color dst, float f)
        {
            return new Color(Vector4F.LerpClamped(src.color, dst.color, f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color Lerp(Color src, Color dst, float f)
        {
            return new Color(Vector4F.Lerp(src.color, dst.color, f));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(Color lhs, Color rhs, float epsilon = MathF.Epsilon)
        {
            return Vector4F.AreApproxEqual(lhs.color, rhs.color, epsilon);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color MulEach(Color lhs, Color rhs)
        {
            return new Color(Vector4F.MulEach(lhs.color, rhs.color));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color operator -(Color color)
        {
            return new Color(-color.color);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color operator +(Color lhs, Color rhs)
        {
            return new Color(lhs.color + rhs.color);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color operator -(Color lhs, Color rhs)
        {
            return new Color(lhs.color - rhs.color);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color operator *(Color color, float f)
        {
            return new Color(color.color * f);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Color operator /(Color color, float f)
        {
            return new Color(color.color / f);
        }

        public bool Equals(Color other)
        {
            return color.Equals(other.color);
        }

        public override bool Equals(object obj)
        {
            return obj is Color other && Equals(other);
        }

        public override int GetHashCode()
        {
            return color.GetHashCode();
        }
    }
}
