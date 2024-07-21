#pragma once
#include <FeCore/Containers/HashTables.h>
#include <OsGPU/Memory/IDeviceMemory.h>
#include <OsGPU/Resource/ITransientResourceHeap.h>
#include <OsGPU/Resource/ResourceCache.h>

namespace FE::Osmium
{
    class IDevice;

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

    class TransientResourceHeapBase : public ITransientResourceHeap
    {
    protected:
        struct RegisteredResourceInfo
        {
            Memory::RefCountedObjectBase* Resource = nullptr;
            NullableHandle Handle = NullableHandle::Null();
            USize Size = 0;

            inline RegisteredResourceInfo() = default;

            inline RegisteredResourceInfo(Memory::RefCountedObjectBase* resource, NullableHandle handle, USize size)
            {
                Resource = resource;
                Handle = handle;
                Size = size;
            }
        };

        TransientResourceHeapDesc m_Desc;
        IDevice* m_Device;
        GPULinearAllocator m_Allocator;
        ResourceCache m_Cache;
        Rc<IDeviceMemory> m_Memory;
        UInt32 m_CreatedResourceCount = 0;
        festd::unordered_dense_map<UInt64, RegisteredResourceInfo> m_RegisteredResources;

        TransientResourceHeapBase(IDevice& dev, const TransientResourceHeapDesc& desc);

        virtual Rc<IDeviceMemory> AllocateMemoryImpl() = 0;
        virtual NullableHandle AllocateResourceMemory(const BufferDesc& desc, USize& byteSize) = 0;
        virtual NullableHandle AllocateResourceMemory(const ImageDesc& desc, USize& byteSize) = 0;

        void ReleaseResource(UInt64 resourceID, TransientResourceType resourceType);

    public:
        FE_CLASS_RTTI(TransientResourceHeapBase, "78A92015-D187-4E83-A4FF-00724C02E7DD");

        ~TransientResourceHeapBase() override = default;

        void Allocate() override;
        Rc<IImage> CreateImage(const TransientImageDesc& desc) override;
        Rc<IBuffer> CreateBuffer(const TransientBufferDesc& desc) override;
        Rc<IImage> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) override;
        Rc<IBuffer> CreateBuffer(const TransientBufferDesc& desc, TransientResourceAllocationStats& stats) override;

        void ReleaseImage(UInt64 resourceID) override;
        void ReleaseBuffer(UInt64 resourceID) override;
    };
} // namespace FE::Osmium
