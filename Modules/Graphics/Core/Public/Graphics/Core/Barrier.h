#pragma once
#include <FeCore/Base/BaseMath.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
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
        kIndirectArgument = 1 << 3,
        kRenderTarget = 1 << 4,
        kShaderRead = 1 << 5,
        kShaderWrite = 1 << 6,
        kDepthStencilRead = 1 << 7,
        kDepthStencilWrite = 1 << 8,
        kCopySource = 1 << 9,
        kCopyDest = 1 << 10,
        kResolveSource = 1 << 11,
        kResolveDest = 1 << 12,
        kShadingRateSource = 1 << 13,
        kAccelerationStructureRead = 1 << 14,
        kAccelerationStructureWrite = 1 << 15,

        kCopySourceAndDest = kCopySource | kCopyDest,

        kCommonResourceAccessMask = kShaderRead | kShaderWrite | kCopySource | kCopyDest,
        kAllTextureAccessMask = kRenderTarget | kDepthStencilRead | kDepthStencilWrite | kResolveSource | kResolveDest
            | kShadingRateSource | kCommonResourceAccessMask,
        kAllBufferAccessMask = kIndexBuffer | kVertexBuffer | kConstantBuffer | kIndirectArgument | kAccelerationStructureRead
            | kAccelerationStructureWrite | kCommonResourceAccessMask,

        kReadAccessMask = kIndexBuffer | kVertexBuffer | kConstantBuffer | kIndirectArgument | kShaderRead | kDepthStencilRead
            | kCopySource | kResolveSource | kShadingRateSource | kAccelerationStructureRead,
        kWriteAccessMask =
            kRenderTarget | kShaderWrite | kDepthStencilWrite | kCopyDest | kResolveDest | kAccelerationStructureWrite,
    };

    FE_ENUM_OPERATORS(BarrierAccessFlags);


    inline bool IsReadAccess(const BarrierAccessFlags accessFlags)
    {
        return Bit::AllSet(BarrierAccessFlags::kReadAccessMask, accessFlags);
    }


    inline bool IsWriteAccess(const BarrierAccessFlags accessFlags)
    {
        return Bit::AllSet(BarrierAccessFlags::kWriteAccessMask, accessFlags);
    }


    struct GlobalBarrierDesc final
    {
        BarrierSyncFlags m_syncBefore = BarrierSyncFlags::kNone;
        BarrierSyncFlags m_syncAfter = BarrierSyncFlags::kNone;
        BarrierAccessFlags m_accessBefore = BarrierAccessFlags::kNone;
        BarrierAccessFlags m_accessAfter = BarrierAccessFlags::kNone;
    };


    struct TextureBarrierDesc final
    {
        BarrierSyncFlags m_syncBefore = BarrierSyncFlags::kNone;
        BarrierSyncFlags m_syncAfter = BarrierSyncFlags::kNone;
        BarrierAccessFlags m_accessBefore = BarrierAccessFlags::kNone;
        BarrierAccessFlags m_accessAfter = BarrierAccessFlags::kNone;
        BarrierLayout m_layoutBefore = BarrierLayout::kUndefined;
        BarrierLayout m_layoutAfter = BarrierLayout::kUndefined;
        TextureSubresource m_subresource = TextureSubresource::kInvalid;
        DeviceQueueType m_queueBefore = DeviceQueueType::kGraphics;
        DeviceQueueType m_queueAfter = DeviceQueueType::kGraphics;
        Texture* m_texture = nullptr;
    };


    struct BufferBarrierDesc final
    {
        BarrierSyncFlags m_syncBefore = BarrierSyncFlags::kNone;
        BarrierSyncFlags m_syncAfter = BarrierSyncFlags::kNone;
        BarrierAccessFlags m_accessBefore = BarrierAccessFlags::kNone;
        BarrierAccessFlags m_accessAfter = BarrierAccessFlags::kNone;
        DeviceQueueType m_queueBefore = DeviceQueueType::kGraphics;
        DeviceQueueType m_queueAfter = DeviceQueueType::kGraphics;
        Buffer* m_buffer = nullptr;
    };
} // namespace FE::Graphics::Core
