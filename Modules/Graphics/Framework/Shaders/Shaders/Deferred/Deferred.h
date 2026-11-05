#pragma once
#include <Shaders/Deferred/Tiling.h>
#include <Shaders/Lighting/ViewLighting.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct DeferredConstants final
    {
        ConstantsPtr<ViewData> m_view;
        ConstantsPtr<ViewLighting> m_lighting;
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

FE_HOST_END_NAMESPACE
