#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/RenderTarget.h>
#include <Graphics/Core/ResourcePool.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Common
{
    struct FrameGraphResourcePool final : public Memory::RefCountedObjectBase
    {
        FrameGraphResourcePool(Core::ResourcePool* pool);
        ~FrameGraphResourcePool() override = default;

        FE_RTTI_Class(FrameGraphResourcePool, "4D13381A-FD8A-4368-B301-2680EE48E082");

        void Reset();

        Core::RenderTarget* CreateRenderTarget(Env::Name name, const Core::ImageDesc& desc);
        Core::Buffer* CreateBuffer(Env::Name name, const Core::BufferDesc& desc);

    private:
        struct ImageInfo final
        {
            Rc<Core::RenderTarget> m_image;
            uint64_t m_descHash = 0;
        };

        struct BufferInfo final
        {
            Rc<Core::Buffer> m_buffer;
            uint64_t m_descHash = 0;
        };

        SegmentedVector<ImageInfo> m_createdImages;
        SegmentedVector<BufferInfo> m_createdBuffers;

        festd::unordered_dense_map<uint64_t, Rc<Core::RenderTarget>> m_imagesMap;
        festd::unordered_dense_map<uint64_t, Rc<Core::Buffer>> m_buffersMap;

        Core::ResourcePool* m_resourcePool = nullptr;
    };
} // namespace FE::Graphics::Common
