#pragma once
#include <FeCore/Math/Rect.h>
#include <Graphics/Core/Texture.h>

//! @brief Declare RTTI for a pass data type without specifying a UUID.
//!
//! Persistant UUIDs are only important for serialized types. For pass data, any ID will do.
#define FE_DECLARE_PASS_DATA(typename) FE_RTTI_Reflect(typename, "Random")


namespace FE::Graphics::Core
{
    struct GraphicsPipeline;
    struct ComputePipeline;


    struct PassGraphicsPipeline final
    {
        const GraphicsPipeline* m_pipeline = nullptr;

        PassGraphicsPipeline() = default;
        PassGraphicsPipeline(const GraphicsPipeline* pipeline)
            : m_pipeline(pipeline)
        {
        }
    };


    struct PassComputePipeline final
    {
        const ComputePipeline* m_pipeline = nullptr;

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
    };


    struct PassTextureAccess final
    {
        Texture* m_texture = nullptr;
        BarrierSyncFlags m_syncFlags = BarrierSyncFlags::kNone;
        BarrierAccessFlags m_accessFlags = BarrierAccessFlags::kNone;
        BarrierLayout m_layout = BarrierLayout::kUndefined;
        TextureSubresource m_subresource = TextureSubresource::kInvalid;
    };


    struct PassColorTarget final
    {
        TextureView m_target = TextureView::kInvalid;

        PassColorTarget() = default;

        PassColorTarget(const TextureView target)
            : m_target(target)
        {
        }
    };


    struct PassDepthTarget final
    {
        TextureView m_target = TextureView::kInvalid;

        PassDepthTarget() = default;

        PassDepthTarget(const TextureView target)
            : m_target(target)
        {
        }
    };


    struct PassViewport final
    {
        RectF m_rect{ kForceInit };

        PassViewport() = default;
        PassViewport(const RectF& rect)
            : m_rect(rect)
        {
        }
    };


    struct PassScissor final
    {
        RectInt m_rect{ kForceInit };

        PassScissor() = default;
        PassScissor(const RectInt& rect)
            : m_rect(rect)
        {
        }
    };
} // namespace FE::Graphics::Core

FE_RTTI_Reflect(FE::Graphics::Core::PassBufferAccess, "6FF3F59E-AEBB-49E8-9B1D-13A9580808FF");
FE_RTTI_Reflect(FE::Graphics::Core::PassTextureAccess, "9F23330E-8C01-4119-A957-0D9A808C7273");
FE_RTTI_Reflect(FE::Graphics::Core::PassColorTarget, "1D24000B-33B4-4BAF-8825-9602CDB08CC8");
FE_RTTI_Reflect(FE::Graphics::Core::PassDepthTarget, "D11CE881-3D3C-47AF-A050-346482677316");
FE_RTTI_Reflect(FE::Graphics::Core::PassGraphicsPipeline, "D90ED870-C966-443F-9BFE-1D753AEEDFBF");
FE_RTTI_Reflect(FE::Graphics::Core::PassComputePipeline, "BDE9AB4A-D506-4B19-9612-3FB7E5133EEA");
FE_RTTI_Reflect(FE::Graphics::Core::PassViewport, "3722E747-AABE-49D8-9D95-ADC6D080D8E7");
FE_RTTI_Reflect(FE::Graphics::Core::PassScissor, "EE4CA91A-4D2F-455F-B9A2-F8F897463D3E");
