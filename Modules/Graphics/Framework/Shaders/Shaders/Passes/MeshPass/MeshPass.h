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


    struct DepthPassDesc final
    {
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;
        Core::PassBufferAccess m_instanceIndices;
        Core::PassBufferAccess m_arguments;

        FE_RTTI_Reflect("A86B4026-4C36-45CE-B6E7-0431BF656B84");
    };


    struct OpaquePassDesc final
    {
        Core::PassColorTarget m_colorTarget;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;
        Core::PassBufferAccess m_instanceIndices;
        Core::PassBufferAccess m_arguments;

        FE_RTTI_Reflect("36F8EE1D-C760-4977-A8CD-A8CC79F66803");
    };

FE_HOST_END_NAMESPACE
