#pragma once
#include <Graphics/Core/InputStreamLayout.h>

namespace FE::Graphics::Core
{
    struct MeshInfo final
    {
        InputStreamLayout m_layout;
    };


    struct MeshLodInfo final
    {
        uint32_t m_vertexCount;
        uint32_t m_indexCount;
        uint32_t m_meshletCount;
        uint32_t m_primitiveCount;
    };


    struct MeshletHeader final
    {
        uint32_t m_vertexCount : 8;
        uint32_t m_vertexOffset : 24;
        uint32_t m_primitiveCount : 8;
        uint32_t m_primitiveOffset : 24;

        static MeshletHeader Pack(const uint32_t vertexCount, const uint32_t vertexOffset, const uint32_t primitiveCount,
                                  const uint32_t primitiveOffset)
        {
            FE_AssertDebug(vertexCount <= (1 << 8) - 1 && vertexOffset <= (1 << 24) - 1);
            FE_AssertDebug(primitiveCount <= (1 << 8) - 1 && primitiveOffset <= (1 << 24) - 1);

            MeshletHeader header;
            header.m_vertexCount = vertexCount;
            header.m_vertexOffset = vertexOffset;
            header.m_primitiveCount = primitiveCount;
            header.m_primitiveOffset = primitiveOffset;
            return header;
        }
    };


    struct PackedTriangle final
    {
        uint32_t m_index0 : 10;
        uint32_t m_index1 : 10;
        uint32_t m_index2 : 10;
        uint32_t m_padding : 2;

        static PackedTriangle Pack(const uint32_t index0, const uint32_t index1, const uint32_t index2)
        {
            FE_AssertDebug(index0 <= (1 << 10) - 1 && index1 <= (1 << 10) - 1 && index2 <= (1 << 10) - 1);

            PackedTriangle triangle;
            triangle.m_index0 = index0;
            triangle.m_index1 = index1;
            triangle.m_index2 = index2;
            triangle.m_padding = 0;
            return triangle;
        }
    };
} // namespace FE::Graphics::Core
