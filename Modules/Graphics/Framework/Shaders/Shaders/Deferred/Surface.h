#pragma once
#include <Shaders/Deferred/Tiling.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct SurfaceConstants final
    {
        ByteAddressBufferDescriptor m_view;
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


    struct SurfacePassDesc final
    {
        SurfaceConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;
        FE_RTTI_Reflect("0708B35E-6919-4FCA-8CCC-5D42D907644F");
    };


    struct SurfaceDirectPassDesc final
    {
        SurfaceConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        FE_RTTI_Reflect("98E1DBB0-D1BA-4F87-A446-12463850EE06");
    };
FE_HOST_END_NAMESPACE
