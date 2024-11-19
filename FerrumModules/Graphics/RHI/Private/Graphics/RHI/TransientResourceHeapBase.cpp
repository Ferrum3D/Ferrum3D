#include <Graphics/RHI/Device.h>
#include <Graphics/RHI/TransientResourceHeapBase.h>

namespace FE::Graphics::RHI
{
    RHI::ResultCode TransientResourceHeapBase::Init(const TransientResourceHeapDesc& desc)
    {
        m_desc = desc;
        return RHI::ResultCode::kSuccess;
    }


    void TransientResourceHeapBase::Allocate()
    {
        m_memory = AllocateMemoryImpl();
        m_allocator.Init(m_desc.m_heapSize);
        m_cache.SetCapacity(m_desc.m_cacheSize);
    }


    Rc<Image> TransientResourceHeapBase::CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats)
    {
        FE_AssertMsg(m_desc.m_typeFlags == TransientResourceType::kImage, "Transient heap type is not compatible");
        size_t allocationSize;
        auto address = AllocateResourceMemory(desc.m_descriptor, allocationSize);
        if (address.IsNull())
        {
            return nullptr;
        }

        FE_AssertMsg(m_createdResourceCount < m_cache.Capacity(), "Resource cache overflow");
        m_createdResourceCount++;

        size_t descHash = 0;
        HashCombine(descHash, desc, address);

        Image* result;
        if (auto* cached = m_cache.FindObject(descHash))
        {
            result = fe_assert_cast<Image*>(cached);
        }
        else
        {
            Rc newImage = Env::GetServiceProvider()->ResolveRequired<Image>();
            newImage->Init(desc.m_name, desc.m_descriptor);
            result = newImage.Get();
            m_cache.AddObject(descHash, result);

            DeviceMemorySlice memory{};
            memory.m_memory = m_memory.Get();
            memory.m_byteSize = allocationSize;
            memory.m_byteOffset = address.ToOffset();
            result->BindMemory(memory);
        }

        m_registeredResources[desc.m_resourceID] = RegisteredResourceInfo(result, address, allocationSize);

        stats.m_minOffset = address.ToOffset();
        stats.m_maxOffset = address.ToOffset() + allocationSize - 1;
        return result;
    }


    Rc<Buffer> TransientResourceHeapBase::CreateBuffer(const TransientBufferDesc& desc, TransientResourceAllocationStats& stats)
    {
        FE_AssertMsg(m_desc.m_typeFlags == TransientResourceType::kBuffer, "Transient heap type is not compatible");
        size_t allocationSize;
        auto address = AllocateResourceMemory(desc.m_descriptor, allocationSize);
        if (address.IsNull())
        {
            return nullptr;
        }

        FE_AssertMsg(m_createdResourceCount < m_cache.Capacity(), "Resource cache overflow");
        m_createdResourceCount++;

        size_t descHash = 0;
        HashCombine(descHash, desc, address);

        Buffer* result;
        if (auto* cached = m_cache.FindObject(descHash))
        {
            result = fe_assert_cast<Buffer*>(cached);
        }
        else
        {
            Rc newBuffer = Env::GetServiceProvider()->ResolveRequired<Buffer>();
            newBuffer->Init(desc.m_name, desc.m_descriptor);

            result = newBuffer.Get();
            m_cache.AddObject(descHash, result);

            DeviceMemorySlice memory{};
            memory.m_memory = m_memory.Get();
            memory.m_byteSize = allocationSize;
            memory.m_byteOffset = address.ToOffset();
            result->BindMemory(memory);
        }

        m_registeredResources[desc.m_resourceID] = RegisteredResourceInfo(result, address, allocationSize);

        stats.m_minOffset = address.ToOffset();
        stats.m_maxOffset = address.ToOffset() + allocationSize - 1;
        return result;
    }


    void TransientResourceHeapBase::ReleaseImage(uint64_t resourceID)
    {
        ReleaseResource(resourceID, TransientResourceType::kImage);
    }


    void TransientResourceHeapBase::ReleaseBuffer(uint64_t resourceID)
    {
        ReleaseResource(resourceID, TransientResourceType::kBuffer);
    }


    void TransientResourceHeapBase::ReleaseResource(uint64_t resourceID, TransientResourceType resourceType)
    {
        FE_AssertMsg(resourceType == m_desc.m_typeFlags, "Transient heap type is not compatible");
        auto& resource = m_registeredResources[resourceID];
        m_allocator.Deallocate(resource.m_handle, resource.m_size);
        if (--m_createdResourceCount == 0)
        {
            m_allocator.Reset();
        }
    }


    Rc<Image> TransientResourceHeapBase::CreateImage(const TransientImageDesc& desc)
    {
        TransientResourceAllocationStats stats{};
        return CreateImage(desc, stats);
    }


    Rc<Buffer> TransientResourceHeapBase::CreateBuffer(const TransientBufferDesc& desc)
    {
        TransientResourceAllocationStats stats{};
        return CreateBuffer(desc, stats);
    }
} // namespace FE::Graphics::RHI
