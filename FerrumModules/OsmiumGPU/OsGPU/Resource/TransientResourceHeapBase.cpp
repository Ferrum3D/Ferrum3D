#include <OsGPU/Device/IDevice.h>
#include <OsGPU/Resource/TransientResourceHeapBase.h>

namespace FE::Osmium
{
    TransientResourceHeapBase::TransientResourceHeapBase(IDevice& dev, const TransientResourceHeapDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
    }

    void TransientResourceHeapBase::Allocate()
    {
        m_Memory = AllocateMemoryImpl();
        m_Allocator.Init(m_Desc.HeapSize);
        m_Cache.SetCapacity(m_Desc.CacheSize);
    }

    Rc<IImage> TransientResourceHeapBase::CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats)
    {
        FE_ASSERT_MSG(m_Desc.TypeFlags == TransientResourceType::Image, "Transient heap type is not compatible");
        size_t allocationSize;
        auto address = AllocateResourceMemory(desc.Descriptor, allocationSize);
        if (address.IsNull())
        {
            return nullptr;
        }

        FE_ASSERT_MSG(m_CreatedResourceCount < m_Cache.Capacity(), "Resource cache overflow");
        m_CreatedResourceCount++;

        size_t descHash = 0;
        HashCombine(descHash, desc, address);

        IImage* result;
        if (auto* cached = m_Cache.FindObject(descHash))
        {
            result = fe_assert_cast<IImage*>(cached);
        }
        else
        {
            auto newImage = m_Device->CreateImage(desc.Descriptor);
            result = newImage.Get();
            m_Cache.AddObject(descHash, result);

            DeviceMemorySlice memory{};
            memory.Memory = m_Memory.Get();
            memory.ByteSize = allocationSize;
            memory.ByteOffset = address.ToOffset();
            result->BindMemory(memory);
        }

        m_RegisteredResources[desc.ResourceID] = RegisteredResourceInfo(result, address, allocationSize);

        stats.MinOffset = address.ToOffset();
        stats.MaxOffset = address.ToOffset() + allocationSize - 1;
        return result;
    }

    Rc<IBuffer> TransientResourceHeapBase::CreateBuffer(const TransientBufferDesc& desc, TransientResourceAllocationStats& stats)
    {
        FE_ASSERT_MSG(m_Desc.TypeFlags == TransientResourceType::Buffer, "Transient heap type is not compatible");
        size_t allocationSize;
        auto address = AllocateResourceMemory(desc.Descriptor, allocationSize);
        if (address.IsNull())
        {
            return nullptr;
        }

        FE_ASSERT_MSG(m_CreatedResourceCount < m_Cache.Capacity(), "Resource cache overflow");
        m_CreatedResourceCount++;

        size_t descHash = 0;
        HashCombine(descHash, desc, address);

        IBuffer* result;
        if (auto* cached = m_Cache.FindObject(descHash))
        {
            result = fe_assert_cast<IBuffer*>(cached);
        }
        else
        {
            auto newBuffer = m_Device->CreateBuffer(desc.Descriptor);
            result = newBuffer.Get();
            m_Cache.AddObject(descHash, result);

            DeviceMemorySlice memory{};
            memory.Memory = m_Memory.Get();
            memory.ByteSize = allocationSize;
            memory.ByteOffset = address.ToOffset();
            result->BindMemory(memory);
        }

        m_RegisteredResources[desc.ResourceID] = RegisteredResourceInfo(result, address, allocationSize);

        stats.MinOffset = address.ToOffset();
        stats.MaxOffset = address.ToOffset() + allocationSize - 1;
        return result;
    }

    void TransientResourceHeapBase::ReleaseImage(uint64_t resourceID)
    {
        ReleaseResource(resourceID, TransientResourceType::Image);
    }

    void TransientResourceHeapBase::ReleaseBuffer(uint64_t resourceID)
    {
        ReleaseResource(resourceID, TransientResourceType::Buffer);
    }

    void TransientResourceHeapBase::ReleaseResource(uint64_t resourceID, TransientResourceType resourceType)
    {
        FE_ASSERT_MSG(resourceType == m_Desc.TypeFlags, "Transient heap type is not compatible");
        auto& resource = m_RegisteredResources[resourceID];
        m_Allocator.Deallocate(resource.Handle, resource.Size);
        if (--m_CreatedResourceCount == 0)
        {
            m_Allocator.Reset();
        }
    }

    Rc<IImage> TransientResourceHeapBase::CreateImage(const TransientImageDesc& desc)
    {
        TransientResourceAllocationStats stats{};
        return CreateImage(desc, stats);
    }

    Rc<IBuffer> TransientResourceHeapBase::CreateBuffer(const TransientBufferDesc& desc)
    {
        TransientResourceAllocationStats stats{};
        return CreateBuffer(desc, stats);
    }
} // namespace FE::Osmium
