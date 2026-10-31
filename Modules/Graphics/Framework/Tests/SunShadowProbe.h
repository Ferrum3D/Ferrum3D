#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::SunShadowProbe)

    FE_CONSTEXPR uint32_t kThreadGroupSize = 8;
    FE_CONSTEXPR uint32_t kFillMode = 0;
    FE_CONSTEXPR uint32_t kReadMode = 1;

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

        FE_RTTI_Reflect("35180F46-4341-484F-9043-9A49464808B7");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("F695EDB3-F62D-42ED-920B-299D4F33F956");
    };

#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines);
#endif

FE_HOST_END_NAMESPACE
