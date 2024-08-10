#pragma once
#include <OsGPU/Buffer/IBuffer.h>
#include <OsGPU/Image/IImage.h>

namespace FE::Osmium
{
    enum class TransientResourceType
    {
        None,
        Image,
        Buffer,
        RenderTarget
    };

    struct TransientResourceHeapDesc
    {
        uint64_t HeapSize = 512 * 1024;
        uint64_t Alignment = 256;
        int32_t CacheSize = 256;
        TransientResourceType TypeFlags = TransientResourceType::Buffer;
    };

    struct TransientImageDesc
    {
        ImageDesc Descriptor;
        uint64_t ResourceID = 0;

        inline TransientImageDesc() = default;

        inline TransientImageDesc(const ImageDesc& descriptor, uint64_t resourceId)
            : Descriptor(descriptor)
            , ResourceID(resourceId)
        {
        }
    };

    struct TransientBufferDesc
    {
        BufferDesc Descriptor;
        uint64_t ResourceID = 0;

        inline TransientBufferDesc() = default;

        inline TransientBufferDesc(const BufferDesc& descriptor, uint64_t resourceId)
            : Descriptor(descriptor)
            , ResourceID(resourceId)
        {
        }
    };

    struct TransientResourceAllocationStats
    {
        uint64_t MinOffset;
        uint64_t MaxOffset;
    };

    class IImage;
    class IBuffer;

    class ITransientResourceHeap : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(ITransientResourceHeap, "DA9D43A4-0EB2-45A4-96B4-714D89A34242");
        ~ITransientResourceHeap() override = default;

        inline virtual void Allocate() = 0;

        [[nodiscard]] virtual Rc<IImage> CreateImage(const TransientImageDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IBuffer> CreateBuffer(const TransientBufferDesc& desc) = 0;

        [[nodiscard]] virtual Rc<IImage> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) = 0;

        [[nodiscard]] virtual Rc<IBuffer> CreateBuffer(const TransientBufferDesc& desc,
                                                       TransientResourceAllocationStats& stats) = 0;

        virtual void ReleaseImage(uint64_t resourceID) = 0;
        virtual void ReleaseBuffer(uint64_t resourceID) = 0;
    };
} // namespace FE::Osmium

FE_MAKE_HASHABLE(FE::Osmium::TransientImageDesc, , value.Descriptor, value.ResourceID);
FE_MAKE_HASHABLE(FE::Osmium::TransientBufferDesc, , value.Descriptor, value.ResourceID);
