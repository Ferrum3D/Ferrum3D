#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::ImGuiPass)

    //! Matches ImDrawVert: position, texture coordinate and packed RGBA8 color.
    FE_CONSTEXPR uint32_t kVertexByteSize = 20;


    struct DrawConstants final
    {
        float2 m_scale;
        float2 m_translation;
        ByteAddressBufferDescriptor m_vertices;
        ByteAddressBufferDescriptor m_indices;
        Texture2DDescriptor<float4> m_texture;
        SamplerDescriptor m_sampler;
        uint32_t m_firstIndex;
        uint32_t m_vertexOffset;

        FE_RTTI_Reflect("AA33A036-1CBD-4537-882F-465E1CEF215E");
    };


    struct TextureUploadConstants final
    {
        ByteAddressBufferDescriptor m_source;
        RWTexture2DDescriptor<float4> m_target;
        uint2 m_offset;
        uint2 m_size;
        uint32_t m_bytesPerPixel;

        FE_RTTI_Reflect("4ACED814-F36A-4A91-B35C-83220229A7CD");
    };


    struct TextureUploadPassDesc final
    {
        TextureUploadConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("841AE091-DA76-4CA7-B3E4-E640715966B2");
    };

FE_HOST_END_NAMESPACE
