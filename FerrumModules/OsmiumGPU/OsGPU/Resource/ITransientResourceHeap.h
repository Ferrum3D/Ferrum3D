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
        UInt64 HeapSize = 512 * 1024;
        UInt64 Alignment = 256;
        Int32 CacheSize = 256;
        TransientResourceType TypeFlags = TransientResourceType::Buffer;
    };

    struct TransientImageDesc
    {
        ImageDesc Descriptor;
        UInt64 ResourceID = 0;

        inline TransientImageDesc() = default;

        inline TransientImageDesc(const ImageDesc& descriptor, UInt64 resourceId)
            : Descriptor(descriptor)
            , ResourceID(resourceId)
        {
        }
    };

    struct TransientBufferDesc
    {
        BufferDesc Descriptor;
        UInt64 ResourceID = 0;

        inline TransientBufferDesc() = default;

        inline TransientBufferDesc(const BufferDesc& descriptor, UInt64 resourceId)
            : Descriptor(descriptor)
            , ResourceID(resourceId)
        {
        }
    };

    struct TransientResourceAllocationStats
    {
        UInt64 MinOffset;
        UInt64 MaxOffset;
    };

    class IImage;
    class IBuffer;

    class ITransientResourceHeap : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(ITransientResourceHeap, "DA9D43A4-0EB2-45A4-96B4-714D89A34242");
        ~ITransientResourceHeap() override = default;

        inline virtual void Allocate() = 0;

        [[nodiscard]] virtual Rc<IImage> CreateImage(const TransientImageDesc& desc) = 0;
        [[nodiscard]] virtual Rc<IBuffer> CreateBuffer(const TransientBufferDesc& desc) = 0;

        [[nodiscard]] virtual Rc<IImage> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) = 0;

        [[nodiscard]] virtual Rc<IBuffer> CreateBuffer(const TransientBufferDesc& desc,
                                                       TransientResourceAllocationStats& stats) = 0;

        virtual void ReleaseImage(UInt64 resourceID) = 0;
        virtual void ReleaseBuffer(UInt64 resourceID) = 0;
    };
} // namespace FE::Osmium

FE_MAKE_HASHABLE(FE::Osmium::TransientImageDesc, , value.Descriptor, value.ResourceID);
FE_MAKE_HASHABLE(FE::Osmium::TransientBufferDesc, , value.Descriptor, value.ResourceID);
