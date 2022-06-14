using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public sealed class Adapter : DeviceObject
    {
        public string Name => desc.Name;
        public AdapterType Type => (AdapterType)desc.Type;
        private readonly Desc desc;

        public Adapter(IntPtr handle) : base(handle)
        {
            GetDescNative(handle, out desc);
        }

        public Device CreateDevice()
        {
            return new Device(CreateDeviceNative(Handle));
        }

        public override string ToString()
        {
            return $"{Type} GPU adapter {Name}";
        }

        [DllImport("OsmiumBindings", EntryPoint = "IAdapter_CreateDevice")]
        private static extern IntPtr CreateDeviceNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IAdapter_GetDesc")]
        private static extern void GetDescNative(IntPtr self, out Desc desc);

        [DllImport("OsmiumBindings", EntryPoint = "IAdapter_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct Desc
        {
            public readonly string Name;
            public readonly int Type;
        }
    }
}
