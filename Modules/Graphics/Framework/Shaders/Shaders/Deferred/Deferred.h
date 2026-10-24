#pragma once
#include <Shaders/Deferred/Tiling.h>
#include <Shaders/Lighting/GlobalLighting.h>
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct DeferredConstants final
    {
        StructuredBufferDescriptor<ViewData> m_view;
        GlobalLightingBinding m_lighting;
        Texture2DDescriptor<uint32_t> m_g0;
        Texture2DDescriptor<uint32_t> m_g1;
        Texture2DDescriptor<uint32_t> m_g2;
        Texture2DDescriptor<uint32_t> m_g3;
        Texture2DDescriptor<float> m_depth;
        RWTexture2DDescriptor<float3> m_output;
        StructuredBufferDescriptor<uint32_t> m_pixels;
        StructuredBufferDescriptor<WaveCommand> m_waves;
        StructuredBufferDescriptor<ShaderRange> m_ranges;
        uint2 m_resolution;
        uint32_t m_tilesPerRow;
        uint32_t m_shaderSlot;
        uint32_t m_shaderCount;
        FE_RTTI_Reflect("B2A55DB1-8534-475D-A6E0-5C4C025861FD");
    };


    struct DeferredPassDesc final
    {
        DeferredConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;
        FE_RTTI_Reflect("FC55E539-C1FD-4D45-9AC9-5E31D9C623E1");
    };


    struct DeferredDirectPassDesc final
    {
        DeferredConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        FE_RTTI_Reflect("906AB659-157B-461D-929F-127F5203B714");
    };
FE_HOST_END_NAMESPACE
