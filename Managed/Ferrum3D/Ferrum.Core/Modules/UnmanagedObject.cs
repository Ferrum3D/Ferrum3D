using System;

namespace Ferrum.Core.Modules
{
    public abstract class UnmanagedObject : IDisposable
    {
        public IntPtr Handle { get; private protected set; }
        public bool IsOwned { get; set; } = true;

        protected UnmanagedObject(IntPtr handle)
        {
            Handle = handle;
        }

        public void Dispose()
        {
            ReleaseHandle();
            GC.SuppressFinalize(this);
        }

        public IntPtr Detach()
        {
            var result = Handle;
            Handle = IntPtr.Zero;
            return result;
        }

        private void ReleaseHandle()
        {
            if (Handle == IntPtr.Zero || !IsOwned)
            {
                return;
            }

            ReleaseUnmanagedResources();
            Handle = IntPtr.Zero;
        }

        protected abstract void ReleaseUnmanagedResources();

        ~UnmanagedObject()
        {
            ReleaseHandle();
        }
    }
}
