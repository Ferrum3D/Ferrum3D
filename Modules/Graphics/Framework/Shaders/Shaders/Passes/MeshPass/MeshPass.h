#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)

    struct Constants final
    {
        float4x4 m_viewProjection;
        MeshInstanceTable::Instance m_meshInstanceTable;
        MeshGroupTable::Instance m_meshGroupTable;
        MeshLodInfoTable::Instance m_meshLodInfoTable;
        MaterialInstanceTable::Instance m_materialInstanceTable;
        StructuredBufferDescriptor<uint32_t> m_instanceIndices;
        uint32_t m_firstInstance;
        uint32_t m_meshletCount;
        uint32_t m_meshletX;

        FE_RTTI_Reflect("9565427F-FE72-49CD-A99E-2A4082D638F7");
    };

FE_HOST_END_NAMESPACE
