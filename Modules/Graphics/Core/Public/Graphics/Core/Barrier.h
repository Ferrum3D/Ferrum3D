#pragma once
#include <FeCore/Base/BaseMath.h>
#include <Graphics/Core/ImageBase.h>

namespace FE::Graphics::Core
{
    struct Buffer;
    struct Texture;


    enum class BarrierLayout : uint32_t
    {
        kUndefined,
        kRenderTarget,
        kShaderRead,
        kShaderReadWrite,
        kDepthStencilRead,
        kDepthStencilWrite,
        kCopySource,
        kCopyDest,
        kResolveSource,
        kResolveDest,
        kShadingRateSource,
        kPresentSource,
    };


    enum class BarrierSyncFlags : uint32_t
    {
        kNone = 0,
        kAll = 1 << 0,
        kExecuteIndirect = 1 << 1,
        kVertexInput = 1 << 2,
        kAmplificationShading = 1 << 3,
        kMeshShading = 1 << 4,
        kVertexShading = 1 << 5,
        kPixelShading = 1 << 6,
        kDepthStencil = 1 << 7,
        kRenderTarget = 1 << 8,
        kComputeShading = 1 << 9,
        kCopy = 1 << 10,
        kResolve = 1 << 11,
        kHost = 1 << 12,
    };

    FE_ENUM_OPERATORS(BarrierSyncFlags);


    enum class BarrierAccessFlags : uint32_t
    {
        kNone = 0,
        kIndexBuffer = 1 << 0,
        kVertexBuffer = 1 << 1,
        kConstantBuffer = 1 << 2,
        kRenderTarget = 1 << 3,
        kShaderRead = 1 << 4,
        kShaderWrite = 1 << 5,
        kDepthStencilRead = 1 << 6,
        kDepthStencilWrite = 1 << 7,
        kCopySource = 1 << 8,
        kCopyDest = 1 << 9,
        kResolveSource = 1 << 10,
        kResolveDest = 1 << 11,
        kShadingRateSource = 1 << 12,
    };

    FE_ENUM_OPERATORS(BarrierAccessFlags);


    struct GlobalBarrierDesc final
    {
        BarrierSyncFlags m_syncBefore : 16;
        BarrierSyncFlags m_syncAfter : 16;
        BarrierAccessFlags m_accessBefore : 16;
        BarrierAccessFlags m_accessAfter : 16;

        GlobalBarrierDesc()
            : m_syncBefore(BarrierSyncFlags::kNone)
            , m_syncAfter(BarrierSyncFlags::kNone)
            , m_accessBefore(BarrierAccessFlags::kNone)
            , m_accessAfter(BarrierAccessFlags::kNone)
        {
        }
    };


    struct TextureBarrierDesc final
    {
        BarrierSyncFlags m_syncBefore : 16;
        BarrierSyncFlags m_syncAfter : 16;
        BarrierAccessFlags m_accessBefore : 16;
        BarrierAccessFlags m_accessAfter : 16;
        BarrierLayout m_layoutBefore : 16;
        BarrierLayout m_layoutAfter : 16;
        ImageSubresource m_subresource = ImageSubresource::kInvalid;
        Texture* m_texture = nullptr;

        TextureBarrierDesc()
            : m_syncBefore(BarrierSyncFlags::kNone)
            , m_syncAfter(BarrierSyncFlags::kNone)
            , m_accessBefore(BarrierAccessFlags::kNone)
            , m_accessAfter(BarrierAccessFlags::kNone)
            , m_layoutBefore(BarrierLayout::kUndefined)
            , m_layoutAfter(BarrierLayout::kUndefined)
        {
        }
    };


    struct BufferBarrierDesc final
    {
        BarrierSyncFlags m_syncBefore : 16;
        BarrierSyncFlags m_syncAfter : 16;
        BarrierAccessFlags m_accessBefore : 16;
        BarrierAccessFlags m_accessAfter : 16;
        Buffer* m_buffer = nullptr;

        BufferBarrierDesc()
            : m_syncBefore(BarrierSyncFlags::kNone)
            , m_syncAfter(BarrierSyncFlags::kNone)
            , m_accessBefore(BarrierAccessFlags::kNone)
            , m_accessAfter(BarrierAccessFlags::kNone)
        {
        }
    };
} // namespace FE::Graphics::Core
