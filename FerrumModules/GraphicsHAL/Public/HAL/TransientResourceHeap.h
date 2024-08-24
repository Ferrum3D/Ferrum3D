#pragma once
#include <FeCore/Strings/FixedString.h>
#include <HAL/Buffer.h>
#include <HAL/DeviceObject.h>
#include <HAL/Image.h>

namespace FE::Graphics::HAL
{
    enum class TransientResourceType
    {
        None,
        Image,
        Buffer,
        RenderTarget
    };


    struct TransientResourceHeapDesc final
    {
        uint64_t HeapSize = 512 * 1024;
        uint64_t Alignment = 256;
        int32_t CacheSize = 256;
        TransientResourceType TypeFlags = TransientResourceType::Buffer;
    };


    struct TransientImageDesc final
    {
        FixStr32 Name;
        ImageDesc Descriptor;
        uint64_t ResourceID = 0;

        inline TransientImageDesc() = default;

        inline TransientImageDesc(StringSlice name, const ImageDesc& descriptor, uint64_t resourceId)
            : Name(name)
            , Descriptor(descriptor)
            , ResourceID(resourceId)
        {
        }
    };


    struct TransientBufferDesc final
    {
        FixStr32 Name;
        BufferDesc Descriptor;
        uint64_t ResourceID = 0;

        inline TransientBufferDesc() = default;

        inline TransientBufferDesc(StringSlice name, const BufferDesc& descriptor, uint64_t resourceId)
            : Name(name)
            , Descriptor(descriptor)
            , ResourceID(resourceId)
        {
        }
    };


    struct TransientResourceAllocationStats final
    {
        uint64_t MinOffset;
        uint64_t MaxOffset;
    };


    class Image;
    class Buffer;

    class TransientResourceHeap : public DeviceObject
    {
    public:
        FE_RTTI_Class(TransientResourceHeap, "DA9D43A4-0EB2-45A4-96B4-714D89A34242");

        ~TransientResourceHeap() override = default;

        virtual ResultCode Init(const TransientResourceHeapDesc& desc) = 0;

        inline virtual void Allocate() = 0;

        [[nodiscard]] virtual Rc<Image> CreateImage(const TransientImageDesc& desc) = 0;
        [[nodiscard]] virtual Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc) = 0;

        [[nodiscard]] virtual Rc<Image> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) = 0;

        [[nodiscard]] virtual Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc,
                                                      TransientResourceAllocationStats& stats) = 0;

        virtual void ReleaseImage(uint64_t resourceID) = 0;
        virtual void ReleaseBuffer(uint64_t resourceID) = 0;
    };
} // namespace FE::Graphics::HAL

FE_MAKE_HASHABLE(FE::Graphics::HAL::TransientImageDesc, , value.Descriptor, value.ResourceID);
FE_MAKE_HASHABLE(FE::Graphics::HAL::TransientBufferDesc, , value.Descriptor, value.ResourceID);
