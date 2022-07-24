using System.Runtime.InteropServices;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU.Descriptors
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct DescriptorSize
    {
        public readonly uint DescriptorCount;
        public readonly ShaderResourceType ResourceType;

        public DescriptorSize(int descriptorCount, ShaderResourceType resourceType)
        {
            DescriptorCount = (uint)descriptorCount;
            ResourceType = resourceType;
        }
    }
}
