using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    public partial struct Quaternion
    {
        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_CreateRotationX")]
        private static extern void CreateRotationXNative(float angle, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_CreateRotationY")]
        private static extern void CreateRotationYNative(float angle, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_CreateRotationZ")]
        private static extern void CreateRotationZNative(float angle, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_FromAxisAngle")]
        private static extern void FromAxisAngleNative(float x, float y, float z, float angle, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_FromAxisAngles")]
        private static extern void FromEulerAnglesNative(float x, float y, float z, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_GetAxisAngle")]
        private static extern void GetAxisAngleNative(in Quaternion self, IntPtr axis, out float angle);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_GetEulerAngles")]
        private static extern void GetEulerAnglesNative(in Quaternion self, IntPtr result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_Lerp")]
        private static extern void LerpNative(in Quaternion src, in Quaternion dst, float f, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_SLerp")]
        private static extern void SLerpNative(in Quaternion src, in Quaternion dst, float f, out Quaternion result);

        [DllImport("FeCoreBindings", EntryPoint = "Quaternion_Multiply")]
        private static extern void MultiplyNative(in Quaternion src, in Quaternion dst, out Quaternion result);
    }
}
