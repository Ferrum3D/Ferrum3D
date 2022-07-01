using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public partial class CommandBuffer : UnmanagedObject
    {
        public CommandBuffer(IntPtr handle) : base(handle)
        {
        }

        public Builder Begin()
        {
            return new Builder(this);
        }

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
