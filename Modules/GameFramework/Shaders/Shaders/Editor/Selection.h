#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Editor)

#if FE_HOST
    using namespace FE::Graphics;
#endif

    struct SelectionConstants final
    {
        Texture2DDescriptor<uint2> m_visibility;
        RWTexture2DDescriptor<float3> m_color;
        StructuredBufferDescriptor<uint32_t> m_selected;
        RWStructuredBufferDescriptor<uint32_t> m_pickResult;
        uint2 m_resolution;
        uint2 m_pickPosition;
        uint32_t m_selectedCount;
        uint32_t m_pick;

        FE_RTTI_Reflect("B48FC0E3-7C7C-467C-AC57-5493B02B094F");
    };


    struct SelectionPassDesc final
    {
        SelectionConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("F53F9A17-5534-4C7B-9460-EBEC36D7C7BD");
    };

FE_HOST_END_NAMESPACE
