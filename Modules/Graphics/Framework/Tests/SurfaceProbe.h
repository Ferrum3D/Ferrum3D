#pragma once
#include <Shaders/Deferred/Tiling.h>
FE_HOST_BEGIN_NAMESPACE(FE::Graphics::SurfaceProbe)
#if FE_HOST
    using Visibility::ShaderRange;
    using Visibility::WaveCommand;
#endif
    struct ProbeConstants final
    {
        RWTexture2DDescriptor<uint2> m_visibility;
        RWTexture2DDescriptor<uint32_t> m_g0;
        RWStructuredBufferDescriptor<uint32_t> m_marks;
        StructuredBufferDescriptor<uint32_t> m_pixels;
        StructuredBufferDescriptor<WaveCommand> m_waves;
        StructuredBufferDescriptor<ShaderRange> m_ranges;
        uint2 m_resolution;
        uint32_t m_instanceBase;
        uint32_t m_groupBase;
        uint32_t m_groupCount;
        uint32_t m_shaderSlot;
        FE_RTTI_Reflect("F662B233-4348-405D-B073-B404D73FD160");
    };


    struct ProbePassDesc final
    {
        ProbeConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        FE_RTTI_Reflect("5A08E833-8551-4C21-90D3-A000C7859D49");
    };


    struct ProbeIndirectDesc final
    {
        ProbeConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;
        FE_RTTI_Reflect("4F199EED-7ABB-441F-863A-558556D1BCE1");
    };
#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines,
               bool largeDispatch = false);
#endif
FE_HOST_END_NAMESPACE
