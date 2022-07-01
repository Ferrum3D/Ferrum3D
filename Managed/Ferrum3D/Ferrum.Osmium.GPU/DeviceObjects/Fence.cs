using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Fence : UnmanagedObject
    {
        public FenceState State => GetStateNative(Handle);

        public Fence(IntPtr handle) : base(handle)
        {
        }

        public void SignalOnCpu()
        {
            SignalOnCPUNative(Handle);
        }

        public void WaitOnCpu()
        {
            WaitOnCPUNative(Handle);
        }

        public void Reset()
        {
            ResetNative(Handle);
        }

        [DllImport("OsGPUBindings", EntryPoint = "IFence_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IFence_SignalOnCPU")]
        private static extern void SignalOnCPUNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IFence_WaitOnCPU")]
        private static extern void WaitOnCPUNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IFence_Reset")]
        private static extern void ResetNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IFence_GetState")]
        private static extern FenceState GetStateNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        public enum FenceState
        {
            Signaled,
            Reset
        }
    }
}
