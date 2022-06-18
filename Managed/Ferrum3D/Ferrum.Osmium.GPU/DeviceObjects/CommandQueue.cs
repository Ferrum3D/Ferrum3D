using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class CommandQueue : UnmanagedObject
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
