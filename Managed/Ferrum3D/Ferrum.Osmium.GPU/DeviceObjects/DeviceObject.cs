using System;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public abstract class DeviceObject : IDisposable
    {
        public IntPtr Handle { get; private set; }

        protected DeviceObject(IntPtr handle)
        {
            Handle = handle;
        }

        public void Dispose()
        {
            ReleaseHandle();
            GC.SuppressFinalize(this);
        }

        private void ReleaseHandle()
        {
            if (Handle == IntPtr.Zero)
            {
                return;
            }

            ReleaseUnmanagedResources();
            Handle = IntPtr.Zero;
        }

        protected abstract void ReleaseUnmanagedResources();

        ~DeviceObject()
        {
            ReleaseHandle();
        }
    }
}
