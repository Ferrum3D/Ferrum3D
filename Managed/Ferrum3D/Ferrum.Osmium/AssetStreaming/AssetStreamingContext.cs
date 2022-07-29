using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.AssetStreaming
{
    public struct AssetStreamingContext
    {
        public Device Device { get; }
        public CommandBuffer.Builder CommandBuffer { get; }

        public AssetStreamingContext(Device device, CommandBuffer.Builder commandBuffer)
        {
            Device = device;
            CommandBuffer = commandBuffer;
        }
    }
}
