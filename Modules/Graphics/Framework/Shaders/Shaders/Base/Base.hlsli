#pragma once
#include <Shaders/Base/BaseTypes.hlsli>
#include <Shaders/Base/Descriptors.hlsli>

namespace InlineMeshes
{
    namespace Quad
    {
        struct Vertex
        {
            float4 m_position;
            float2 m_texCoord;
        };

        Vertex GetVertex(const uint32_t index)
        {
            const Vertex vertices[] = {
                // first triangle
                { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                { { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                { { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                // second triangle
                { { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                { { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
                { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
            };
            return vertices[index];
        }
    } // namespace Quad
} // namespace InlineMeshes
