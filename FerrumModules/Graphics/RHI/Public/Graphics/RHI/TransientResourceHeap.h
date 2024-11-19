#pragma once
#include <FeCore/Strings/FixedString.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/Image.h>

namespace FE::Graphics::RHI
{
    enum class TransientResourceType : uint32_t
    {
        kNone,
        kImage,
        kBuffer,
        kRenderTarget,
    };


    struct TransientResourceHeapDesc final
    {
        uint64_t m_heapSize = 512 * 1024;
        uint64_t m_alignment = 256;
        int32_t m_cacheSize = 256;
        TransientResourceType m_typeFlags = TransientResourceType::kBuffer;
    };


    struct TransientImageDesc final
    {
        Env::Name m_name;
        ImageDesc m_descriptor;
        uint64_t m_resourceID = 0;

        TransientImageDesc() = default;

        TransientImageDesc(StringSlice name, const ImageDesc& descriptor, uint64_t resourceId)
            : m_name(name)
            , m_descriptor(descriptor)
            , m_resourceID(resourceId)
        {
        }
    };


    struct TransientBufferDesc final
    {
        Env::Name m_name;
        BufferDesc m_descriptor;
        uint64_t m_resourceID = 0;

        TransientBufferDesc() = default;

        TransientBufferDesc(StringSlice name, const BufferDesc& descriptor, uint64_t resourceId)
            : m_name(name)
            , m_descriptor(descriptor)
            , m_resourceID(resourceId)
        {
        }
    };


    struct TransientResourceAllocationStats final
    {
        uint64_t m_minOffset;
        uint64_t m_maxOffset;
    };


    struct Image;
    struct Buffer;

    struct TransientResourceHeap : public DeviceObject
    {
        FE_RTTI_Class(TransientResourceHeap, "DA9D43A4-0EB2-45A4-96B4-714D89A34242");

        ~TransientResourceHeap() override = default;

        virtual ResultCode Init(const TransientResourceHeapDesc& desc) = 0;

        virtual void Allocate() = 0;

        [[nodiscard]] virtual Rc<Image> CreateImage(const TransientImageDesc& desc) = 0;
        [[nodiscard]] virtual Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc) = 0;

        [[nodiscard]] virtual Rc<Image> CreateImage(const TransientImageDesc& desc, TransientResourceAllocationStats& stats) = 0;

        [[nodiscard]] virtual Rc<Buffer> CreateBuffer(const TransientBufferDesc& desc,
                                                      TransientResourceAllocationStats& stats) = 0;

        virtual void ReleaseImage(uint64_t resourceID) = 0;
        virtual void ReleaseBuffer(uint64_t resourceID) = 0;
    };
} // namespace FE::Graphics::RHI


FE_MAKE_HASHABLE(FE::Graphics::RHI::TransientImageDesc, , value.m_descriptor, value.m_resourceID);
FE_MAKE_HASHABLE(FE::Graphics::RHI::TransientBufferDesc, , value.m_descriptor, value.m_resourceID);
