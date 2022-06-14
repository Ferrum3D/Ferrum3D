using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    public static class Matrix4x4FBindings
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F Multiply(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            MultiplyNative(ref lhs, ref rhs, out var result);
            return result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F Multiply(Matrix4x4F lhs, Vector4F rhs)
        {
            MultiplyNative(ref lhs, ref rhs, out var result);
            return result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float Determinant(Matrix4x4F lhs)
        {
            return DeterminantNative(ref lhs);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F InverseTransform(Matrix4x4F matrix)
        {
            InverseTransformNative(ref matrix, out var result);
            return result;
        }

        /// <summary>
        ///     Managed implementation of matrix multiplication for reference.
        /// </summary>
        /// <param name="lhs">Left matrix</param>
        /// <param name="rhs">Right matrix</param>
        /// <returns>Result of multiplication</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F MultiplyManaged(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            var result = Matrix4x4F.Zero;
            for (var i = 0; i < 4; ++i)
            for (var j = 0; j < 4; ++j)
            for (var k = 0; k < 4; ++k)
            {
                result[i, j] += lhs[i, k] * rhs[k, j];
            }

            return result;
        }

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_Multiply")]
        private static extern void MultiplyNative(ref Matrix4x4F lhs, ref Matrix4x4F rhs, out Matrix4x4F result);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_VectorMultiply")]
        private static extern void MultiplyNative(ref Matrix4x4F lhs, ref Vector4F rhs, out Vector4F result);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_Determinant")]
        private static extern float DeterminantNative(ref Matrix4x4F matrix);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_InverseTransform")]
        private static extern void InverseTransformNative(ref Matrix4x4F matrix, out Matrix4x4F result);
    }
}
