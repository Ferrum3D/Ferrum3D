#pragma once
#include <FeCore/Allocators/LinearAllocator.h>
#include <OsGPU/Memory/IDeviceMemory.h>
#include <OsGPU/Resource/ITransientResourceHeap.h>
#include <OsGPU/Resource/ResourceCache.h>

namespace FE::Osmium
{
    class IDevice;

    using GPULinearAllocator = GenericLinearAllocator<NullableHandle>;

    class TransientResourceHeapBase : public Object<ITransientResourceHeap>
    {
    protected:
        struct RegisteredResourceInfo
        {
            IObject* Resource     = nullptr;
            NullableHandle Handle = NullableHandle::Null();
            USize Size            = 0;

            inline RegisteredResourceInfo() = default;

            inline RegisteredResourceInfo(IObject* resource, NullableHandle handle, USize size)
            {
                Resource = resource;
                Handle   = handle;
                Size     = size;
            }
        };

        TransientResourceHeapDesc m_Desc;
        IDevice* m_Device;
        GPULinearAllocator m_Allocator;
        ResourceCache m_Cache;
        Shared<IDeviceMemory> m_Memory;
        UInt32 m_CreatedResourceCount = 0;
        UnorderedMap<UInt64, RegisteredResourceInfo> m_RegisteredResources;

        TransientResourceHeapBase(IDevice& dev, const TransientResourceHeapDesc& desc);

        virtual Shared<IDeviceMemory> AllocateMemoryImpl() = 0;
        virtual NullableHandle AllocateResourceMemory(const BufferDesc& desc, USize& byteSize) = 0;
        virtual NullableHandle AllocateResourceMemory(const ImageDesc& desc, USize& byteSize) = 0;

        void ReleaseResource(UInt64 resourceID, TransientResourceType resourceType);

    public:
        FE_CLASS_RTTI(TransientResourceHeapBase, "78A92015-D187-4E83-A4FF-00724C02E7DD");

        ~TransientResourceHeapBase() override = default;

        void Allocate() override;
        Shared<IImage> CreateImage(const TransientImageDesc& desc) override;
        Shared<IBuffer> CreateBuffer(const TransientBufferDesc& desc) override;
        Shared<IImage> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) override;
        Shared<IBuffer> CreateBuffer(const TransientBufferDesc& desc, TransientResourceAllocationStats& stats) override;

        void ReleaseImage(UInt64 resourceID) override;
        void ReleaseBuffer(UInt64 resourceID) override;
    };
} // namespace FE::Osmium
