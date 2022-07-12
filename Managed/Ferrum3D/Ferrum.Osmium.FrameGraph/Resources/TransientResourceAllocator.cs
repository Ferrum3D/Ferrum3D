using System;
using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Containers;
using Ferrum.Core.Math;
using Ferrum.Osmium.GPU.DeviceObjects;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public class TransientResourceAllocator : IDisposable
    {
        public Desc Descriptor { get; }

        private readonly DisposableList<TransientResourceHeap> heapPages = new();
        private readonly Dictionary<ulong, TransientResourceHeap> resourceIdToHeapMap = new();
        private readonly Device device;

        private ulong bytesAllocated;

        private float PageGrowFactor => Descriptor.AllocationPolicy == AllocationPolicy.AllocatePages
            ? Descriptor.PageGrowFactor
            : 1.0f;

        public TransientResourceAllocator(Device device, in Desc desc)
        {
            Descriptor = desc;
            this.device = device;
            AddHeapPage();
        }

        public Image AllocateImage(in TransientImageDesc desc)
        {
            var result = null as Image;
            var heap = null as TransientResourceHeap;
            foreach (var page in heapPages)
            {
                if (page.TryCreateImage(desc, out result))
                {
                    heap = page;
                    break;
                }
            }

            if (result is null)
            {
                switch (Descriptor.AllocationPolicy)
                {
                    case AllocationPolicy.FixedSize:
                        throw new OutOfMemoryException("Transient resource allocator couldn't allocate a new heap for " +
                                                       "an image: Fixed size allocation policy was used");
                    case AllocationPolicy.AllocatePages:
                        AddHeapPage();
                        heap = heapPages.Last();
                        heap.TryCreateImage(desc, out result);
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }

            if (result is null)
            {
                throw new OutOfMemoryException("Transient image allocation failed");
            }

            resourceIdToHeapMap[desc.ResourceID] = heap;
            return result;
        }

        public Buffer AllocateBuffer(in TransientBufferDesc desc)
        {
            var result = null as Buffer;
            var heap = null as TransientResourceHeap;
            foreach (var page in heapPages)
            {
                if (page.TryCreateBuffer(desc, out result))
                {
                    heap = page;
                    break;
                }
            }

            if (result is null)
            {
                switch (Descriptor.AllocationPolicy)
                {
                    case AllocationPolicy.FixedSize:
                        throw new OutOfMemoryException("Transient resource allocator couldn't allocate a new heap for a " +
                                                       "buffer: Fixed size allocation policy was used");
                    case AllocationPolicy.AllocatePages:
                        AddHeapPage();
                        heap = heapPages.Last();
                        heap.TryCreateBuffer(desc, out result);
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }

            if (result is null)
            {
                throw new OutOfMemoryException("Transient buffer allocation failed");
            }

            resourceIdToHeapMap[desc.ResourceID] = heap;
            return result;
        }

        public void ReleaseImage(ulong resourceId)
        {
            if (!resourceIdToHeapMap.TryGetValue(resourceId, out var heap))
            {
                throw new ArgumentException($"The resource {resourceId} was not allocated with this allocator");
            }

            heap.ReleaseImage(resourceId);
            resourceIdToHeapMap.Remove(resourceId);
        }

        public void ReleaseBuffer(ulong resourceId)
        {
            if (!resourceIdToHeapMap.TryGetValue(resourceId, out var heap))
            {
                throw new ArgumentException($"The resource {resourceId} was not allocated with this allocator");
            }

            heap.ReleaseBuffer(resourceId);
            resourceIdToHeapMap.Remove(resourceId);
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        private void AddHeapPage()
        {
            if (bytesAllocated == Descriptor.MemoryBudget)
            {
                throw new OutOfMemoryException("Transient resource allocator is out of specified memory budget");
            }

            // TODO: check if the resource we try to allocate will fit in remaining memory
            var size = (ulong)(Descriptor.InitialPageSize * MathF.Pow(PageGrowFactor, heapPages.Count));
            size = Math.Min(size, Descriptor.MemoryBudget - bytesAllocated);
            bytesAllocated += size;

            var pageDesc = new TransientResourceHeap.Desc(size, Descriptor.Alignment, Descriptor.PageCacheSize,
                Descriptor.AllocatedResourceType);
            heapPages.Add(device.CreateTransientResourceHeap(pageDesc));
        }

        private void ReleaseUnmanagedResources()
        {
            heapPages.Dispose();
        }

        public enum AllocationPolicy
        {
            FixedSize,
            AllocatePages
        }

        public readonly struct Desc
        {
            public readonly TransientResourceType AllocatedResourceType;
            public readonly AllocationPolicy AllocationPolicy;
            public readonly ulong InitialPageSize;
            public readonly ulong Alignment;
            public readonly float PageGrowFactor;
            public readonly ulong MemoryBudget;
            public readonly int PageCacheSize;

            public Desc(TransientResourceType allocatedResourceType,
                AllocationPolicy allocationPolicy = AllocationPolicy.AllocatePages, ulong initialPageSize = 512 * 1024,
                ulong alignment = 256, float pageGrowFactor = 1.0f, ulong memoryBudget = ulong.MaxValue, int pageCacheSize = 256)
            {
                AllocatedResourceType = allocatedResourceType;
                AllocationPolicy = allocationPolicy;
                InitialPageSize = initialPageSize;
                Alignment = alignment;
                PageGrowFactor = pageGrowFactor;
                MemoryBudget = memoryBudget;
                PageCacheSize = pageCacheSize;
            }
        }

        ~TransientResourceAllocator()
        {
            ReleaseUnmanagedResources();
        }
    }
}
