using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public enum AdapterType
    {
        None,
        Integrated,
        Discrete,
        Virtual,
        Cpu
    }

    public sealed class Adapter : IDisposable
    {
        public string Name => desc.Name;
        public AdapterType Type => (AdapterType)desc.Type;
        private IntPtr handle;
        private readonly Desc desc;

        public Adapter(IntPtr handle)
        {
            this.handle = handle;
            GetDescNative(handle, out desc);
        }

        public Device CreateDevice()
        {
            return new Device(CreateDeviceNative(handle));
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        [DllImport("OsmiumBindings", EntryPoint = "IAdapter_CreateDevice")]
        private static extern IntPtr CreateDeviceNative(IntPtr self);
        
        [DllImport("OsmiumBindings", EntryPoint = "IAdapter_GetDesc")]
        private static extern void GetDescNative(IntPtr self, out Desc desc);

        [DllImport("OsmiumBindings", EntryPoint = "IAdapter_Destruct")]
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

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct Desc
        {
            public readonly string Name;
            public readonly int Type;
        }

        public override string ToString()
        {
            return $"{Type} GPU adapter {Name}";
        }

        ~Adapter()
        {
            ReleaseUnmanagedResources();
        }
    }
}
