#pragma once
#include <Graphics/Core/ImageSubresource.h>

namespace FE::Graphics::Core
{
    enum class ResourceState : uint32_t
    {
        kUndefined,
        kCommon,
        kVertexBuffer,
        kConstantBuffer,
        kIndexBuffer,
        kRenderTarget,
        kUnorderedAccess,
        kDepthWrite,
        kDepthRead,
        kShaderResource,
        kIndirectArgument,
        kTransferWrite,
        kTransferRead,
        kPresent,
        kAutomatic,
        kCount,
    };


    struct Image;
    struct Buffer;

    struct ImageBarrierDesc final
    {
        Image* m_image = nullptr;
        ImageSubresourceRange m_subresourceRange;
        ResourceState m_stateBefore = ResourceState::kAutomatic;
        ResourceState m_stateAfter = ResourceState::kUndefined;
    };


    struct BufferBarrierDesc final
    {
        Buffer* m_buffer = nullptr;
        uint64_t m_offset = 0;
        uint64_t m_size = 0;
        ResourceState m_stateBefore = ResourceState::kAutomatic;
        ResourceState m_stateAfter = ResourceState::kUndefined;
    };
} // namespace FE::Graphics::Core
