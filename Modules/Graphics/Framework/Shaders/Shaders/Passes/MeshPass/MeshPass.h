#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)

    struct Constants final
    {
        BufferPointer m_meshInstanceTable;
        BufferPointer m_meshGroupTable;
        BufferPointer m_meshLodInfoTable;
        uint32_t m_instanceIndex = 0;
        uint3 m_padding{ 0, 0, 0 };
        float4x4 m_viewProjection;
        float4 m_baseColor;
    };

FE_HOST_END_NAMESPACE

FE_DECLARE_PASS_DATA(FE::Graphics::MeshPass::Constants);
