#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)

    struct Constants final
    {
        float4x4 m_viewProjection;
        float4 m_baseColor;
        MeshInstanceTable::Instance m_meshInstanceTable;
        MeshGroupTable::Instance m_meshGroupTable;
        MeshLodInfoTable::Instance m_meshLodInfoTable;
        uint32_t m_instanceIndex;
        uint3 m_padding;
        uint2 m_padding2;

        FE_RTTI_Reflect("9565427F-FE72-49CD-A99E-2A4082D638F7");
    };

FE_HOST_END_NAMESPACE
