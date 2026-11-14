#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::ScreenTraceProbe)

    FE_CONSTEXPR uint32_t kFillMode = 0;
    FE_CONSTEXPR uint32_t kTraceMode = 1;
    FE_CONSTEXPR uint32_t kThreadGroupSize = 8;

    //! Bits of the result of a pixel: whether ScreenTrace and the reference implementation found an occluder.
    FE_CONSTEXPR uint32_t kTraceOccluded = 1;
    FE_CONSTEXPR uint32_t kReferenceOccluded = 2;

    //! Fills m_target with a stepped depth, or traces one ray per pixel of m_depth with both implementations.
    struct Constants final
    {
        ConstantsPtr<ViewData> m_view;
        Texture2DDescriptor<float> m_depth;
        RWTexture2DDescriptor<float> m_target;
        RWStructuredBufferDescriptor<uint32_t> m_output;
        uint32_t m_mode;
        uint2 m_size;

        FE_RTTI_Reflect("D58ED7E7-5907-4A4D-B73A-366365D87F17");
    };

#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines);
#endif

FE_HOST_END_NAMESPACE
