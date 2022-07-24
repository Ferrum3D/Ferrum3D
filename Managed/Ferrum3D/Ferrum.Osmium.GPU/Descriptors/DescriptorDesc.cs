using System.Runtime.InteropServices;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.Shaders;

namespace Ferrum.Osmium.GPU.Descriptors
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct DescriptorDesc
    {
        public readonly ShaderResourceType ResourceType;
        public readonly ShaderStageFlags Stage;
        public readonly uint Count;

        public DescriptorDesc(ShaderResourceType resourceType, ShaderStageFlags stage, uint count)
        {
            ResourceType = resourceType;
            Stage = stage;
            Count = count;
        }
    }
}
