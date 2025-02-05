#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/Image.h>
#include <Graphics/RHI/ResourcePool.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::RHI
{
    struct FrameGraphResourcePool final : public Memory::RefCountedObjectBase
    {
        FrameGraphResourcePool(ResourcePool* pool);
        ~FrameGraphResourcePool() override = default;

        FE_RTTI_Class(FrameGraphResourcePool, "4D13381A-FD8A-4368-B301-2680EE48E082");

        void Reset();

        festd::expected<Image*, ResultCode> CreateImage(Env::Name name, const ImageDesc& desc);
        festd::expected<Buffer*, ResultCode> CreateBuffer(Env::Name name, const BufferDesc& desc);

    private:
        struct ImageInfo final
        {
            Rc<Image> m_image;
            uint64_t m_descHash = 0;
        };

        struct BufferInfo final
        {
            Rc<Buffer> m_buffer;
            uint64_t m_descHash = 0;
        };

        SegmentedVector<ImageInfo> m_createdImages;
        SegmentedVector<BufferInfo> m_createdBuffers;

        festd::unordered_dense_map<uint64_t, Rc<Image>> m_imagesMap;
        festd::unordered_dense_map<uint64_t, Rc<Buffer>> m_buffersMap;

        ResourcePool* m_resourcePool = nullptr;
    };
} // namespace FE::Graphics::RHI
