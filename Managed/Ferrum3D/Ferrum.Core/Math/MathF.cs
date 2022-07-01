using System;
using System.Runtime.CompilerServices;

namespace Ferrum.Core.Math
{
    /// <summary>
    ///     Single-precision floating point math.
    /// </summary>
    public static class MathF
    {
        public const float Epsilon = 0.0001f;
        public const float PI = (float)System.Math.PI;
        public const float E = (float)System.Math.E;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Sin(float x)
        {
            return (float)System.Math.Sin(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Cos(float x)
        {
            return (float)System.Math.Cos(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Tan(float x)
        {
            return (float)System.Math.Tan(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Asin(float x)
        {
            return (float)System.Math.Asin(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Acos(float x)
        {
            return (float)System.Math.Acos(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Atan(float x)
        {
            return (float)System.Math.Atan(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Atan2(float y, float x)
        {
            return (float)System.Math.Atan2(y, x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Sqrt(float x)
        {
            return (float)System.Math.Sqrt(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Pow(float x, float y)
        {
            return (float)System.Math.Pow(x, y);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Exp(float x)
        {
            return (float)System.Math.Exp(x);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(float lhs, float rhs, float epsilon = Epsilon)
        {
            return System.Math.Abs(lhs - rhs) < epsilon;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Clamp(float value, float min, float max)
        {
            return System.Math.Max(min, System.Math.Min(value, max));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Saturate(float value)
        {
            return Clamp(value, 0, 1);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float NextFloat(this Random random)
        {
            return (float)random.NextDouble();
        }
    }
}
