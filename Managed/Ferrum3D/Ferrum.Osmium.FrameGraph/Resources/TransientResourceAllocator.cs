using System;
using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Containers;
using Ferrum.Core.Math;
using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.GPU.DeviceObjects;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public class TransientResourceAllocator : IDisposable
    {
        public Desc Descriptor { get; }

        private readonly DisposableList<TransientResourceHeap> heapPages = new();
        private readonly List<AliasedResourceTracker> pageResourceTrackers = new();
        private readonly Dictionary<ulong, ResourceInfo> resourceIdToInfoMap = new();
        private readonly Device device;

        private ulong bytesAllocated;

        private readonly struct ResourceInfo
        {
            public readonly int HeapIndex;
            public readonly FrameGraphRenderPass Pass;
            public readonly ulong HeapOffsetMin;
            public readonly ulong HeapOffsetMax;
            public readonly Resource Resource;

            public ResourceInfo(int heapIndex, FrameGraphRenderPass pass, ulong heapOffsetMin, ulong heapOffsetMax,
                Resource resource)
            {
                HeapIndex = heapIndex;
                Pass = pass;
                HeapOffsetMin = heapOffsetMin;
                HeapOffsetMax = heapOffsetMax;
                Resource = resource;
            }
        }

        private float PageGrowFactor => Descriptor.AllocationPolicy == AllocationPolicy.AllocatePages
            ? Descriptor.PageGrowFactor
            : 1.0f;

        public TransientResourceAllocator(Device device, in Desc desc)
        {
            Descriptor = desc;
            this.device = device;
            AddHeapPage();
        }

        public Image AllocateImage(in Image.Desc desc, ulong id, FrameGraphRenderPass pass)
        {
            return AllocateImage(new TransientImageDesc(desc, id), pass);
        }

        public Image AllocateImage(in TransientImageDesc desc, FrameGraphRenderPass pass)
        {
            var result = null as Image;
            var heapIndex = 0;
            var allocationStats = new TransientResourceHeap.AllocationStats();
            for (var i = 0; i < heapPages.Count; i++)
            {
                if (heapPages[i].TryCreateImage(desc, out result, out allocationStats))
                {
                    heapIndex = i;
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
                        heapIndex = heapPages.Count - 1;
                        heapPages[heapIndex].TryCreateImage(desc, out result, out allocationStats);
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }

            if (result is null)
            {
                throw new OutOfMemoryException("Transient image allocation failed");
            }

            resourceIdToInfoMap[desc.ResourceID] =
                new ResourceInfo(heapIndex, pass, allocationStats.MinOffset, allocationStats.MaxOffset, result);
            return result;
        }

        public Buffer AllocateBuffer(in Buffer.Desc desc, ulong id, FrameGraphRenderPass pass)
        {
            return AllocateBuffer(new TransientBufferDesc(desc, id), pass);
        }

        public Buffer AllocateBuffer(in TransientBufferDesc desc, FrameGraphRenderPass pass)
        {
            var result = null as Buffer;
            var heapIndex = 0;
            var allocationStats = new TransientResourceHeap.AllocationStats();
            for (var i = 0; i < heapPages.Count; i++)
            {
                if (heapPages[i].TryCreateBuffer(desc, out result, out allocationStats))
                {
                    heapIndex = i;
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
                        heapIndex = heapPages.Count - 1;
                        heapPages[heapIndex].TryCreateBuffer(desc, out result, out allocationStats);
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }

            if (result is null)
            {
                throw new OutOfMemoryException("Transient buffer allocation failed");
            }

            resourceIdToInfoMap[desc.ResourceID] =
                new ResourceInfo(heapIndex, pass, allocationStats.MinOffset, allocationStats.MaxOffset, result);
            return result;
        }

        private void ReleaseResource(ulong resourceId, Action<TransientResourceHeap> resourceReleaser, FrameGraphRenderPass pass)
        {
            if (!resourceIdToInfoMap.TryGetValue(resourceId, out var info))
            {
                throw new ArgumentException($"The resource {resourceId} was not allocated with this allocator");
            }

            pageResourceTrackers[info.HeapIndex]
                .Add(new AliasedResourceDesc(info.Pass, pass, info.HeapOffsetMin, info.HeapOffsetMax, info.Resource));
            resourceReleaser(heapPages[info.HeapIndex]);
            resourceIdToInfoMap.Remove(resourceId);
        }

        public void ReleaseImage(ulong resourceId, FrameGraphRenderPass pass)
        {
            ReleaseResource(resourceId, heap => heap.ReleaseImage(resourceId), pass);
        }

        public void ReleaseBuffer(ulong resourceId, FrameGraphRenderPass pass)
        {
            ReleaseResource(resourceId, heap => heap.ReleaseBuffer(resourceId), pass);
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
            pageResourceTrackers.Add(new AliasedResourceTracker());
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
