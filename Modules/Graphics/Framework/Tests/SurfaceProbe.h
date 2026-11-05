#pragma once
#include <Shaders/Deferred/Tiling.h>
FE_HOST_BEGIN_NAMESPACE(FE::Graphics::SurfaceProbe)
#if FE_HOST
    using Visibility::ShaderRange;
    using Visibility::SurfaceCommandsConstants;
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


#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines,
               bool largeDispatch = false);
#endif
FE_HOST_END_NAMESPACE
