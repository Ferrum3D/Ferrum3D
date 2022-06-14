using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Device : IDisposable
    {
        private IntPtr handle;

        public Device(IntPtr handle)
        {
            this.handle = handle;
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }
        
        [DllImport("OsmiumBindings", EntryPoint = "IDevice_Destruct")]
        private static extern void DestructNative(IntPtr self);

        private void ReleaseUnmanagedResources()
        {
            if (handle == IntPtr.Zero)
            {
                return;
            }

            DestructNative(handle);
            handle = IntPtr.Zero;
        }
        
        ~Device()
        {
            ReleaseUnmanagedResources();
        }
    }
}
