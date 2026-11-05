#pragma once
#include <Shaders/Deferred/Tiling.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct SurfaceConstants final
    {
        AnyConstantsPtr m_view;
        Texture2DDescriptor<uint2> m_visibility;
        StructuredBufferDescriptor<uint32_t> m_pixels;
        StructuredBufferDescriptor<WaveCommand> m_waves;
        StructuredBufferDescriptor<ShaderRange> m_ranges;
        RWTexture2DDescriptor<uint32_t> m_g0;
        RWTexture2DDescriptor<uint32_t> m_g1;
        RWTexture2DDescriptor<uint32_t> m_g2;
        RWTexture2DDescriptor<uint32_t> m_g3;
        uint2 m_resolution;
        uint32_t m_tilesPerRow;
        uint32_t m_shaderSlot;
        FE_RTTI_Reflect("7BE23B88-FFAC-4420-95C8-7593DB85EFF2");
    };

FE_HOST_END_NAMESPACE
