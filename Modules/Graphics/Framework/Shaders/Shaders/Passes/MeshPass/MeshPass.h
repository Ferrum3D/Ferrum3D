#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)

    struct Constants final
    {
        MeshInstanceTable::Instance m_meshInstanceTable;
        MeshGroupTable::Instance m_meshGroupTable;
        MeshLodInfoTable::Instance m_meshLodInfoTable;
        uint32_t m_instanceIndex FE_INIT(0);
        uint3 m_padding FE_INIT({ 0, 0, 0 });
        float4x4 m_viewProjection;
        float4 m_baseColor;
    };

FE_HOST_END_NAMESPACE

FE_DECLARE_PASS_DATA(FE::Graphics::MeshPass::Constants);
