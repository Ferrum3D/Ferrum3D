#pragma once
#include <FeCore/Containers/HashTables.h>
#include <HAL/DeviceMemory.h>
#include <HAL/ResourceCache.h>
#include <HAL/TransientResourceHeap.h>

namespace FE::Graphics::HAL
{
    class Device;

    class GPULinearAllocator final
    {
        size_t m_Size = 0;
        NullableHandle m_Offset = {};

    public:
        void Init(size_t size)
        {
            FE_ASSERT(m_Size == 0);
            m_Size = size;
            Reset();
        }

        NullableHandle Allocate(size_t size, size_t alignment)
        {
            FE_ASSERT(m_Offset.IsValid());
            const size_t address = AlignUp(m_Offset.ToOffset(), alignment);
            m_Offset = NullableHandle::FromOffset(address + size);
            return NullableHandle::FromOffset(address);
        }

        void Deallocate(NullableHandle, size_t)
        {
            // do nothing
        }

        void Reset()
        {
            m_Offset = NullableHandle::Zero();
        }
    };

    class TransientResourceHeapBase : public TransientResourceHeap
    {
    protected:
        struct RegisteredResourceInfo
        {
            Memory::RefCountedObjectBase* Resource = nullptr;
            NullableHandle Handle = NullableHandle::Null();
            size_t Size = 0;

            inline RegisteredResourceInfo() = default;

            inline RegisteredResourceInfo(Memory::RefCountedObjectBase* resource, NullableHandle handle, size_t size)
            {
                Resource = resource;
                Handle = handle;
                Size = size;
            }
        };

        TransientResourceHeapDesc m_Desc;
        Device* m_Device;
        GPULinearAllocator m_Allocator;
        ResourceCache m_Cache;
        Rc<DeviceMemory> m_Memory;
        uint32_t m_CreatedResourceCount = 0;
        festd::unordered_dense_map<uint64_t, RegisteredResourceInfo> m_RegisteredResources;

        virtual Rc<DeviceMemory> AllocateMemoryImpl() = 0;
        virtual NullableHandle AllocateResourceMemory(const BufferDesc& desc, size_t& byteSize) = 0;
        virtual NullableHandle AllocateResourceMemory(const ImageDesc& desc, size_t& byteSize) = 0;

        void ReleaseResource(uint64_t resourceID, TransientResourceType resourceType);

    public:
        FE_RTTI_Class(TransientResourceHeapBase, "78A92015-D187-4E83-A4FF-00724C02E7DD");

        ~TransientResourceHeapBase() override = default;

        HAL::ResultCode Init(const TransientResourceHeapDesc& desc) override;

        void Allocate() override;
        Rc<Image> CreateImage(const TransientImageDesc& desc) override;
        Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc) override;
        Rc<Image> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) override;
        Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc, TransientResourceAllocationStats& stats) override;

        void ReleaseImage(uint64_t resourceID) override;
        void ReleaseBuffer(uint64_t resourceID) override;
    };
} // namespace FE::Graphics::HAL
