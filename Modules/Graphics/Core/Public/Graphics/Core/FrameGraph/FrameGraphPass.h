#pragma once
#include <FeCore/Math/Rect.h>

//! @brief Declare RTTI for a pass data type without specifying a UUID.
//!
//! Persistant UUIDs are only important for serialized types. For pass data, any ID will do.
#define FE_DECLARE_PASS_DATA(typename) FE_RTTI_Reflect(typename, "Random")


namespace FE::Graphics::Core
{
    struct GraphicsPipeline;
    struct ComputePipeline;
    struct Texture;


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


    struct PassColorTarget final
    {
        Texture* m_target = nullptr;
        uint32_t m_explicitIndex = kInvalidIndex;

        PassColorTarget() = default;
        PassColorTarget(Texture* target)
            : m_target(target)
        {
        }

        PassColorTarget(Texture* target, const uint32_t index)
            : m_target(target)
            , m_explicitIndex(index)
        {
        }
    };


    struct PassDepthTarget final
    {
        Texture* m_target = nullptr;

        PassDepthTarget() = default;
        PassDepthTarget(Texture* target)
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

FE_RTTI_Reflect(FE::Graphics::Core::PassColorTarget, "1D24000B-33B4-4BAF-8825-9602CDB08CC8");
FE_RTTI_Reflect(FE::Graphics::Core::PassDepthTarget, "D11CE881-3D3C-47AF-A050-346482677316");
FE_RTTI_Reflect(FE::Graphics::Core::PassGraphicsPipeline, "D90ED870-C966-443F-9BFE-1D753AEEDFBF");
FE_RTTI_Reflect(FE::Graphics::Core::PassComputePipeline, "BDE9AB4A-D506-4B19-9612-3FB7E5133EEA");
FE_RTTI_Reflect(FE::Graphics::Core::PassViewport, "3722E747-AABE-49D8-9D95-ADC6D080D8E7");
FE_RTTI_Reflect(FE::Graphics::Core::PassScissor, "EE4CA91A-4D2F-455F-B9A2-F8F897463D3E");
