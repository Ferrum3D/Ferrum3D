using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Device : DeviceObject
    {
        public Device(IntPtr handle) : base(handle)
        {
        }

        public CommandQueue GetCommandQueue(CommandQueueClass cmdQueueClass)
        {
            return new CommandQueue(GetCommandQueueNative(Handle, (int)cmdQueueClass));
        }

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_GetCommandQueue")]
        private static extern IntPtr GetCommandQueueNative(IntPtr self, int cmdQueueClass);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
