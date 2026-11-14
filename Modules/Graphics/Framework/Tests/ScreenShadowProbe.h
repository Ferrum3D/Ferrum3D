#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::ScreenShadowProbe)

    FE_CONSTEXPR uint32_t kFillMode = 0;
    FE_CONSTEXPR uint32_t kReadMode = 1;
    FE_CONSTEXPR uint32_t kThreadGroupSize = 8;

    //! The probe depth is a floor that faces the camera with a thin slab in front of it. Reverse Z: the slab is closer.
    FE_CONSTEXPR float kFloorDepth = 0.5f;
    FE_CONSTEXPR float kSlabDepth = 0.501f;

    //! Fills m_target with the probe depth, or copies m_source into m_output.
    struct Constants final
    {
        Texture2DDescriptor<float> m_source;
        RWTexture2DDescriptor<float> m_target;
        RWStructuredBufferDescriptor<float> m_output;
        uint32_t m_mode;
        uint2 m_size;
        uint2 m_slabMin;
        uint2 m_slabMax;

        FE_RTTI_Reflect("CCCF743F-56B8-4E6B-8B5A-D70104D6FF5C");
    };

#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines);
#endif

FE_HOST_END_NAMESPACE
