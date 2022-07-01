using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public readonly struct DescriptorWriteSampler
    {
        public readonly Sampler Sampler;
        public readonly int Binding;
        public readonly int ArrayIndex;

        public DescriptorWriteSampler(Sampler sampler, int binding = 0, int arrayIndex = 0)
        {
            Sampler = sampler;
            Binding = binding;
            ArrayIndex = arrayIndex;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly IntPtr Image;
            public readonly uint Binding;
            public readonly uint ArrayIndex;

            public Native(DescriptorWriteSampler write)
            {
                Image = write.Sampler?.Handle ?? IntPtr.Zero;
                Binding = (uint)write.Binding;
                ArrayIndex = (uint)write.ArrayIndex;
            }
        }
    }
}
