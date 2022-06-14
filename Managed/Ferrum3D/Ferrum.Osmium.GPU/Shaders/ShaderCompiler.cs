using System;
using System.Runtime.InteropServices;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU.Shaders
{
    public class ShaderCompiler : DeviceObject
    {
        public ShaderCompiler(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsmiumBindings", EntryPoint = "IShaderCompiler_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
