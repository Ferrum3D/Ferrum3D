#pragma once
#include <Core/Math/Rect.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    struct GraphicsPipeline;
    struct ComputePipeline;


    struct PassGraphicsPipeline final
    {
        const GraphicsPipeline* m_pipeline = nullptr;

        FE_RTTI_Reflect("D90ED870-C966-443F-9BFE-1D753AEEDFBF");

        PassGraphicsPipeline() = default;
        PassGraphicsPipeline(const GraphicsPipeline* pipeline)
            : m_pipeline(pipeline)
        {
        }
    };


    struct PassComputePipeline final
    {
        const ComputePipeline* m_pipeline = nullptr;

        FE_RTTI_Reflect("BDE9AB4A-D506-4B19-9612-3FB7E5133EEA");

        PassComputePipeline() = default;
        PassComputePipeline(const ComputePipeline* pipeline)
            : m_pipeline(pipeline)
        {
        }
    };


    struct PassBufferAccess final
    {
        Buffer* m_buffer = nullptr;
        BarrierSyncFlags m_syncFlags = BarrierSyncFlags::kNone;
        BarrierAccessFlags m_accessFlags = BarrierAccessFlags::kNone;

        FE_RTTI_Reflect("6FF3F59E-AEBB-49E8-9B1D-13A9580808FF");

        PassBufferAccess() = default;
        PassBufferAccess(const BufferView buffer, const BarrierSyncFlags syncFlags, const BarrierAccessFlags accessFlags)
            : m_buffer(buffer.m_resource)
            , m_syncFlags(syncFlags)
            , m_accessFlags(accessFlags)
        {
        }
    };


    struct PassTextureAccess final
    {
        Texture* m_texture = nullptr;
        BarrierSyncFlags m_syncFlags = BarrierSyncFlags::kNone;
        BarrierAccessFlags m_accessFlags = BarrierAccessFlags::kNone;
        BarrierLayout m_layout = BarrierLayout::kUndefined;
        TextureSubresource m_subresource = TextureSubresource::kInvalid;

        FE_RTTI_Reflect("9F23330E-8C01-4119-A957-0D9A808C7273");

        PassTextureAccess() = default;
        PassTextureAccess(const TextureView texture, const BarrierSyncFlags syncFlags, const BarrierAccessFlags accessFlags,
                          const BarrierLayout layout)
            : m_texture(texture.m_resource)
            , m_syncFlags(syncFlags)
            , m_accessFlags(accessFlags)
            , m_layout(layout)
            , m_subresource(texture.m_subresource)
        {
        }
    };


    struct PassColorTarget final
    {
        TextureView m_target = TextureView::kInvalid;

        FE_RTTI_Reflect("1D24000B-33B4-4BAF-8825-9602CDB08CC8");

        PassColorTarget() = default;

        PassColorTarget(const TextureView target)
            : m_target(target)
        {
        }
    };


    struct PassDepthTarget final
    {
        TextureView m_target = TextureView::kInvalid;

        FE_RTTI_Reflect("D11CE881-3D3C-47AF-A050-346482677316");

        PassDepthTarget() = default;

        PassDepthTarget(const TextureView target)
            : m_target(target)
        {
        }
    };


    struct PassViewport final
    {
        RectF m_rect{ kForceInit };

        FE_RTTI_Reflect("3722E747-AABE-49D8-9D95-ADC6D080D8E7");

        PassViewport() = default;
        PassViewport(const RectF& rect)
            : m_rect(rect)
        {
        }
    };


    struct PassScissor final
    {
        RectInt m_rect{ kForceInit };

        FE_RTTI_Reflect("EE4CA91A-4D2F-455F-B9A2-F8F897463D3E");

        PassScissor() = default;
        PassScissor(const RectInt& rect)
            : m_rect(rect)
        {
        }
    };


    struct BufferAccessPassDesc final
    {
        PassBufferAccess m_access;

        FE_RTTI_Reflect("4A168BAD-024C-480F-9333-DE48B324D7F4");
    };


    struct TextureAccessPassDesc final
    {
        PassTextureAccess m_access;

        FE_RTTI_Reflect("009DDC47-BC19-4525-8A64-56403D359C95");
    };

} // namespace FE::Graphics::Core
