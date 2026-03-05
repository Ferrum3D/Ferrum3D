#pragma once
#include <Shaders/Base/Base.h>

namespace Core
{
    struct MeshletHeader
    {
        uint32_t m_vertexCount : 8;
        uint32_t m_vertexOffset : 24;
        uint32_t m_primitiveCount : 8;
        uint32_t m_primitiveOffset : 24;
    };


    struct MeshLodInfo
    {
        uint32_t m_vertexCount;
        uint32_t m_indexCount;
        uint32_t m_meshletCount;
        uint32_t m_primitiveCount;
    };


    struct PackedTriangle
    {
        uint32_t m_index0 : 10;
        uint32_t m_index1 : 10;
        uint32_t m_index2 : 10;
        uint32_t m_padding : 2;
    };
} // namespace Core
