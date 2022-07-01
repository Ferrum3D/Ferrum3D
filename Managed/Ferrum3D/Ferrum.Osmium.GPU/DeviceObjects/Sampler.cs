using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.PipelineStates;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Sampler : UnmanagedObject
    {
        public Sampler(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsGPUBindings", EntryPoint = "ISampler_Destruct")]
        private static extern void DestructNative(IntPtr handle);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Desc
        {
            public readonly SamplerAddressMode AddressMode;
            public readonly SamplerAnisotropy Anisotropy;
            public readonly CompareOp CompareOp;
            public readonly bool CompareEnable;

            public static readonly Desc Default = new Desc(SamplerAddressMode.Repeat, SamplerAnisotropy.MaxSupported,
                CompareOp.Always, false);

            public Desc(SamplerAddressMode addressMode, SamplerAnisotropy anisotropy, CompareOp compareOp,
                bool compareEnable)
            {
                AddressMode = addressMode;
                Anisotropy = anisotropy;
                CompareOp = compareOp;
                CompareEnable = compareEnable;
            }
        }
    }
}
