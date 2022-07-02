using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class CommandQueue : UnmanagedObject
    {
        internal CommandQueue(IntPtr handle) : base(handle)
        {
        }

        public void SubmitBuffers(CommandBuffer buffer, Fence signalFence, SubmitFlags flags)
        {
            SubmitBuffers(new[] { buffer }, signalFence, flags);
        }

        public void SubmitBuffers(IEnumerable<CommandBuffer> buffers, Fence signalFence, SubmitFlags flags)
        {
            SubmitBuffersNative(Handle, NativeArray<IntPtr>.FromObjectCollection(buffers).Detach(),
                signalFence.Handle, flags);
        }

        [DllImport("OsGPUBindings", EntryPoint = "ICommandQueue_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandQueue_SubmitBuffers")]
        private static extern void SubmitBuffersNative(IntPtr self, IntPtr buffers, IntPtr signalFence,
            SubmitFlags flags);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [Flags]
        public enum SubmitFlags
        {
            None = 0,
            FrameBegin = 1 << 0,
            FrameEnd = 1 << 1,
            FrameBeginEnd = FrameBegin | FrameEnd
        }
    }
}
