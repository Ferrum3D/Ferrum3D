#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Shadows/SunShadow.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::SunShadowProbe)

    FE_CONSTEXPR uint32_t kThreadGroupSize = 8;
    FE_CONSTEXPR uint32_t kFillMode = 0;
    FE_CONSTEXPR uint32_t kReadMode = 1;
    FE_CONSTEXPR uint32_t kFillAtlasMode = 2;
    FE_CONSTEXPR uint32_t kSampleMode = 3;
    FE_CONSTEXPR uint32_t kRequestMode = 4;

    //! Fills m_target with m_depth, leaving the first m_backgroundRows rows as background, or copies mip 0 of m_source into
    //! m_output.
    struct Constants final
    {
        RWTexture2DDescriptor<float> m_target;
        Texture2DDescriptor<float> m_source;
        RWStructuredBufferDescriptor<float> m_output;
        uint2 m_size;
        float m_depth;
        uint32_t m_backgroundRows;
        uint32_t m_mode;
        uint32_t m_outputOffset;
        SunShadowBinding m_shadow;
        RWTexture2DDescriptor<uint32_t> m_atlas;
        RWStructuredBufferDescriptor<uint32_t> m_requests;
        float3 m_position;
        uint32_t m_level;

        FE_RTTI_Reflect("35180F46-4341-484F-9043-9A49464808B7");
    };

#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines);
#endif

FE_HOST_END_NAMESPACE
