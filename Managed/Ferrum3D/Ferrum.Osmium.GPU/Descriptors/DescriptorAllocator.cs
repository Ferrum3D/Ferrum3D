using System;
using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Containers;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU.Descriptors
{
    public sealed class DescriptorAllocator : IDisposable
    {
        private readonly Device device;

        private readonly float[] descriptorSizes = new float[(int)ShaderResourceType.Count];
        private readonly DisposableList<DescriptorHeap> freeHeaps;
        private readonly DisposableList<DescriptorHeap> usedHeaps;
        private DescriptorHeap currentHeap;

        public DescriptorAllocator(Device device)
        {
            this.device = device;

            descriptorSizes[(int)ShaderResourceType.ConstantBuffer] = 2.0f;
            descriptorSizes[(int)ShaderResourceType.TextureSrv] = 4.0f;
            descriptorSizes[(int)ShaderResourceType.TextureUav] = 1.0f;
            descriptorSizes[(int)ShaderResourceType.BufferSrv] = 1.0f;
            descriptorSizes[(int)ShaderResourceType.BufferUav] = 1.0f;
            descriptorSizes[(int)ShaderResourceType.Sampler] = 4.0f;
            descriptorSizes[(int)ShaderResourceType.InputAttachment] = 0.5f;
        }

        public DescriptorTable AllocateTable(DescriptorDesc[] descriptors)
        {
            var result = currentHeap?.AllocateDescriptorTable(descriptors);

            if (result is null)
            {
                currentHeap = GetHeap();
                usedHeaps.Add(currentHeap);
                result = currentHeap.AllocateDescriptorTable(descriptors);
            }

            return result;
        }

        public void ResetHeaps()
        {
            foreach (var heap in usedHeaps)
            {
                heap.Reset();
            }

            freeHeaps.AddRange(usedHeaps);
            usedHeaps.Clear();
            currentHeap = null;
        }

        public void Dispose()
        {
            currentHeap = null;
            freeHeaps.Dispose();
            usedHeaps.Dispose();
        }

        private DescriptorHeap GetHeap()
        {
            if (!freeHeaps.Any())
            {
                return CreateHeap(512);
            }

            var result = freeHeaps.Last();
            freeHeaps.RemoveAt(freeHeaps.Count - 1);
            return result;
        }

        private DescriptorHeap CreateHeap(int count)
        {
            var sizes = descriptorSizes
                .Select((x, i) => new DescriptorSize((int)(count * x), (ShaderResourceType)i));

            return device.CreateDescriptorHeap(new DescriptorHeap.Desc(sizes, count));
        }
    }
}
