#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::RasterProbe)

#if FE_HOST
    using MeshPass::MeshViewData;
#endif

    struct Constants final
    {
        StructuredBufferDescriptor<MeshViewData> m_view;
        Texture2DDescriptor<uint2> m_visibility;
        Texture2DDescriptor<float> m_depth;
        Texture2DDescriptor<uint32_t> m_g0;
        Texture2DDescriptor<uint32_t> m_g1;
        Texture2DDescriptor<uint32_t> m_g2;
        RWStructuredBufferDescriptor<uint32_t> m_words;

        FE_RTTI_Reflect("98560760-F872-4ED1-8CF8-A19D363EF058");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("8F252F45-CD62-4035-BFF2-394C62791858");
    };

#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines,
               uint32_t waveLaneCount = 32);
#endif

FE_HOST_END_NAMESPACE
