using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public sealed class Adapter : UnmanagedObject
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

        [DllImport("OsGPUBindings", EntryPoint = "IAdapter_CreateDevice")]
        private static extern IntPtr CreateDeviceNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IAdapter_GetDesc")]
        private static extern void GetDescNative(IntPtr self, out Desc desc);

        [DllImport("OsGPUBindings", EntryPoint = "IAdapter_Destruct")]
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
