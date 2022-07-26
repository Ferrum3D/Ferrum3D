using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    public partial struct Matrix4x4F
    {
        /// <summary>
        ///     Managed implementation of matrix multiplication for reference.
        /// </summary>
        /// <param name="lhs">Left matrix</param>
        /// <param name="rhs">Right matrix</param>
        /// <returns>Result of multiplication</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F MultiplyManaged(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            var result = Zero;
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
        
        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_CreateRotationX")]
        private static extern void CreateRotationXNative(float angle, out Matrix4x4F result);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_CreateRotationY")]
        private static extern void CreateRotationYNative(float angle, out Matrix4x4F result);
        
        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_CreateRotation")]
        private static extern void CreateRotationNative(in Quaternion rotation, out Matrix4x4F result);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_CreateTransform")]
        private static extern void CreateTransformNative(in Quaternion rotation, in Vector4F position, out Matrix4x4F result);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_CreateRotationZ")]
        private static extern void CreateRotationZNative(float angle, out Matrix4x4F result);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_Determinant")]
        private static extern float DeterminantNative(ref Matrix4x4F matrix);

        [DllImport("FeCoreBindings", EntryPoint = "Matrix4x4F_InverseTransform")]
        private static extern void InverseTransformNative(ref Matrix4x4F matrix, out Matrix4x4F result);
    }
}
