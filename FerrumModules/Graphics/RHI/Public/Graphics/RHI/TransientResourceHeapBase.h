#pragma once
#include <festd/unordered_map.h>
#include <Graphics/RHI/DeviceMemory.h>
#include <Graphics/RHI/ResourceCache.h>
#include <Graphics/RHI/TransientResourceHeap.h>

namespace FE::Graphics::RHI
{
    struct Device;

    struct GPULinearAllocator final
    {
        size_t m_size = 0;
        NullableHandle m_offset = {};

    public:
        void Init(size_t size)
        {
            FE_Assert(m_size == 0);
            m_size = size;
            Reset();
        }

        NullableHandle Allocate(size_t size, size_t alignment)
        {
            FE_Assert(m_offset.IsValid());
            const size_t address = AlignUp(m_offset.ToOffset(), alignment);
            m_offset = NullableHandle::FromOffset(address + size);
            return NullableHandle::FromOffset(address);
        }

        void Deallocate(NullableHandle, size_t)
        {
            // do nothing
        }

        void Reset()
        {
            m_offset = NullableHandle::Zero();
        }
    };


    struct TransientResourceHeapBase : public TransientResourceHeap
    {
        FE_RTTI_Class(TransientResourceHeapBase, "78A92015-D187-4E83-A4FF-00724C02E7DD");

        ~TransientResourceHeapBase() override = default;

        RHI::ResultCode Init(const TransientResourceHeapDesc& desc) override;

        void Allocate() override;
        Rc<Image> CreateImage(const TransientImageDesc& desc) override;
        Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc) override;
        Rc<Image> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) override;
        Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc, TransientResourceAllocationStats& stats) override;

        void ReleaseImage(uint64_t resourceID) override;
        void ReleaseBuffer(uint64_t resourceID) override;

    protected:
        struct RegisteredResourceInfo final
        {
            Memory::RefCountedObjectBase* m_resource = nullptr;
            NullableHandle m_handle = NullableHandle::Null();
            size_t m_size = 0;

            RegisteredResourceInfo() = default;

            RegisteredResourceInfo(Memory::RefCountedObjectBase* resource, NullableHandle handle, size_t size)
            {
                m_resource = resource;
                m_handle = handle;
                m_size = size;
            }
        };

        TransientResourceHeapDesc m_desc;
        Device* m_device;
        GPULinearAllocator m_allocator;
        ResourceCache m_cache;
        Rc<DeviceMemory> m_memory;
        uint32_t m_createdResourceCount = 0;
        festd::unordered_dense_map<uint64_t, RegisteredResourceInfo> m_registeredResources;

        virtual Rc<DeviceMemory> AllocateMemoryImpl() = 0;
        virtual NullableHandle AllocateResourceMemory(const BufferDesc& desc, size_t& byteSize) = 0;
        virtual NullableHandle AllocateResourceMemory(const ImageDesc& desc, size_t& byteSize) = 0;

        void ReleaseResource(uint64_t resourceID, TransientResourceType resourceType);
    };
} // namespace FE::Graphics::RHI
