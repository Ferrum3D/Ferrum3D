using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class CommandQueue : DeviceObject
    {
        public CommandQueue(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_Destruct")]
        private static extern void DestructNative(IntPtr self);
        
        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
