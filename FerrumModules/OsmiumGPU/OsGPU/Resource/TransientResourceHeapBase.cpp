#include <OsGPU/Buffer/VKBuffer.h>
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
        GPULinearAllocator::Desc allocatorDesc{};
        allocatorDesc.StartOffset = NullableHandle::Zero();
        allocatorDesc.GCLatency   = 3;
        allocatorDesc.Capacity    = m_Desc.HeapSize;
        m_Allocator.Init(allocatorDesc);
        m_Cache.SetCapacity(m_Desc.CacheSize);
    }

    Shared<IImage> TransientResourceHeapBase::CreateImage(const TransientImageDesc& desc)
    {
        FE_ASSERT_MSG(m_Desc.TypeFlags == TransientResourceType::Image, "Transient heap type is not compatible");
        // We need a way to get image memory size given a descriptor.
        (void)desc;
        FE_UNREACHABLE("Unimplemented!");
        return nullptr;
    }

    Shared<IBuffer> TransientResourceHeapBase::CreateBuffer(const TransientBufferDesc& desc)
    {
        FE_ASSERT_MSG(m_Desc.TypeFlags == TransientResourceType::Buffer, "Transient heap type is not compatible");
        auto address = m_Allocator.Allocate(desc.Descriptor.Size, m_Desc.Alignment, FE_SRCPOS());
        if (address.IsNull())
        {
            return nullptr;
        }

        FE_ASSERT_MSG(m_CreatedResourceCount < m_Cache.Capacity(), "Resource cache overflow");
        m_CreatedResourceCount++;

        size_t descHash = 0;
        HashCombine(descHash, desc, address);

        IBuffer* result;
        if (IObject* cached = m_Cache.FindObject(descHash))
        {
            result = fe_assert_cast<IBuffer*>(cached);
        }
        else
        {
            auto newBuffer = m_Device->CreateBuffer(desc.Descriptor);
            result         = newBuffer.GetRaw();
            m_Cache.AddObject(descHash, result);

            DeviceMemorySlice memory{};
            memory.Memory     = m_Memory;
            memory.ByteSize   = desc.Descriptor.Size;
            memory.ByteOffset = address.ToOffset();
            result->BindMemory(memory);
        }

        m_RegisteredResources[desc.ResourceID] = RegisteredResourceInfo(result, address, desc.Descriptor.Size);

        return result;
    }

    void TransientResourceHeapBase::ReleaseImage(UInt64 resourceID)
    {
        ReleaseResource(resourceID, TransientResourceType::Image);
    }

    void TransientResourceHeapBase::ReleaseBuffer(UInt64 resourceID)
    {
        ReleaseResource(resourceID, TransientResourceType::Buffer);
    }

    void TransientResourceHeapBase::ReleaseResource(UInt64 resourceID, TransientResourceType resourceType)
    {
        FE_ASSERT_MSG(resourceType == m_Desc.TypeFlags, "Transient heap type is not compatible");
        auto& resource = m_RegisteredResources[resourceID];
        m_Allocator.Deallocate(resource.Handle, FE_SRCPOS(), resource.Size);
    }
} // namespace FE::Osmium
