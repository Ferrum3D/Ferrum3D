#pragma once
#include <Shaders/Passes/Visibility/Tiling.h>
FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct DeferredConstants final
    {
        StructuredBufferDescriptor<ViewData> m_view;
        Texture2DDescriptor<uint32_t> m_g0;
        Texture2DDescriptor<uint32_t> m_g1;
        Texture2DDescriptor<uint32_t> m_g2;
        Texture2DDescriptor<uint32_t> m_g3;
        Texture2DDescriptor<float> m_depth;
        RWTexture2DDescriptor<float3> m_output;
        float4x4 m_inverseViewProjection;
        uint2 m_resolution;
        FE_RTTI_Reflect("B2A55DB1-8534-475D-A6E0-5C4C025861FD");
    };
FE_HOST_END_NAMESPACE
