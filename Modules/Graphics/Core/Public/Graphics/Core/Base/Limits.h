#pragma once

namespace FE::Graphics::Core::Limits
{
    namespace Image
    {
        constexpr uint32_t kMaxMipCount = 13;
        constexpr uint32_t kMaxWidth = 1 << kMaxMipCount;
    } // namespace Image


    namespace Pipeline
    {
        constexpr uint32_t kMaxColorAttachments = 8;
        constexpr uint32_t kMaxShaderResourceGroups = 8;
        constexpr uint32_t kMaxSpecializationConstants = 8;
        constexpr uint32_t kMaxPushConstantsByteSize = 128;
        constexpr uint32_t kMaxVertexStreams = 12;
        constexpr uint32_t kMaxStreamChannels = 15;
    } // namespace Pipeline


    namespace Vertex
    {
        constexpr uint32_t kMaxTexCoords = 4;
        constexpr uint32_t kMaxColors = 4;
        constexpr uint32_t kMaxInfluenceBones = 4;
    } // namespace Vertex


    namespace Mesh
    {
        constexpr uint32_t kMaxMeshletVertexCount = 64;
        constexpr uint32_t kMaxMeshletPrimitiveCount = 64;
    } // namespace Mesh
} // namespace FE::Graphics::Core::Limits
