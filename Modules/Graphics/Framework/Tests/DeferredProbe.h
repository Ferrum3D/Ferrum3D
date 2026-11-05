#pragma once
#include <Shaders/Deferred/Deferred.h>
FE_HOST_BEGIN_NAMESPACE(FE::Graphics::DeferredProbe)
    struct Constants final
    {
        RWTexture2DDescriptor<uint32_t> m_g0;
        RWTexture2DDescriptor<uint32_t> m_g1;
        RWTexture2DDescriptor<uint32_t> m_g2;
        RWTexture2DDescriptor<uint32_t> m_g3;
        RWTexture2DDescriptor<float> m_depth;
        Texture2DDescriptor<float3> m_color;
        RWStructuredBufferDescriptor<uint32_t> m_stats;
        ConstantsPtr<ViewData> m_view;
        ConstantsPtr<GlobalLightingData> m_expectedLighting;
        uint2 m_resolution;
        uint32_t m_empty;
        FE_RTTI_Reflect("446E9FDE-3F29-4ACD-9343-DFF9F07B87D1");
    };
#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines);
#endif
FE_HOST_END_NAMESPACE
