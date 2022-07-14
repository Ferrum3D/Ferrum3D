using System;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public class TransientResourceSystem : IDisposable
    {
        private readonly TransientResourceAllocator imageAllocator;
        private readonly TransientResourceAllocator bufferAllocator;

        public TransientResourceSystem(Device device)
        {
            // TODO: make the allocator Desc configurable
            var imageAllocatorDesc = new TransientResourceAllocator.Desc(TransientResourceType.Image);
            imageAllocator = new TransientResourceAllocator(device, imageAllocatorDesc);
            var bufferAllocatorDesc = new TransientResourceAllocator.Desc(TransientResourceType.Buffer);
            bufferAllocator = new TransientResourceAllocator(device, bufferAllocatorDesc);
        }

        public void AllocateResource(FrameGraphResource resource)
        {
            switch (resource)
            {
                case FrameGraphImageResource image:
                    image.RealImage = imageAllocator.AllocateImage(image.Descriptor, image.Id, image.Creator);
                    break;
                case FrameGraphBufferResource buffer:
                    buffer.RealBuffer = bufferAllocator.AllocateBuffer(buffer.Descriptor, buffer.Id, buffer.Creator);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public void DeallocateResource(FrameGraphResource resource)
        {
            switch (resource)
            {
                case FrameGraphImageResource image:
                    imageAllocator.ReleaseImage(image.Id, image.Deleter);
                    break;
                case FrameGraphBufferResource buffer:
                    bufferAllocator.ReleaseBuffer(buffer.Id, buffer.Deleter);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public void Dispose()
        {
            imageAllocator.Dispose();
            bufferAllocator.Dispose();
        }
    }
}
