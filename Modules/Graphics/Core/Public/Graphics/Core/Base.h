#pragma once
#include <cstdint>

namespace FE::Graphics::Core
{
    enum class Format : uint32_t;
    enum class VertexChannelFormat : uint32_t;

    struct Device;
    struct DeviceObject;

    struct Fence;
    struct FenceSyncPoint;

    struct Buffer;
    struct Resource;
    struct Texture;

    struct PipelineFactory;
    struct PipelineBase;
    struct GraphicsPipeline;
    struct ComputePipeline;
    struct DescriptorManager;

    struct AsyncCopyQueue;
    struct ResourcePool;


    namespace Limits
    {
        namespace Image
        {
            constexpr uint32_t kMaxMipCount = 15;
            constexpr uint32_t kMaxWidth = 1 << (kMaxMipCount - 1);
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
    } // namespace Limits
} // namespace FE::Graphics::Core
