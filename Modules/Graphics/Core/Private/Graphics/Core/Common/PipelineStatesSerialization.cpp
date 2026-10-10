#include <Graphics/Core/PipelineStates.h>

#include <Core/Serialization/Serialization.h>

namespace FE::Graphics::Core
{
    Serialization::ResultCode RasterizationState::Serialize(Serialization::SerializationContext& context,
                                                            const RasterizationState& value)
    {
        static_assert(sizeof(RasterizationState) == sizeof(uint32_t));
        if (context.IsBinary())
            return context.RawBytes(value);

        CullingModeFlags cullMode = value.m_cullMode;
        PolygonMode polyMode = value.m_polyMode;

        if (auto object = context.BeginObject())
        {
            object.Field("m_cullMode", cullMode);
            object.Field("m_polyMode", polyMode);
        }

        return context.GetResultCode();
    }


    Serialization::ResultCode RasterizationState::Deserialize(Serialization::DeserializationContext& context,
                                                              RasterizationState& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        CullingModeFlags cullMode = value.m_cullMode;
        PolygonMode polyMode = value.m_polyMode;

        if (auto object = context.BeginObject())
        {
            object.Field("m_cullMode", cullMode);
            object.Field("m_polyMode", polyMode);
        }

        const Serialization::ResultCode result = context.GetResultCode();
        if (result == Serialization::ResultCode::kSuccess)
        {
            value.m_cullMode = cullMode;
            value.m_polyMode = polyMode;
        }
        return result;
    }


    Serialization::ResultCode DepthStencilState::Serialize(Serialization::SerializationContext& context,
                                                           const DepthStencilState& value)
    {
        static_assert(sizeof(DepthStencilState) == sizeof(uint32_t));
        if (context.IsBinary())
            return context.RawBytes(value);

        CompareOp compareOp = value.m_depthCompareOp;
        bool depthTest = value.m_depthTestEnabled;
        bool depthWrite = value.m_depthWriteEnabled;
        bool stencilTest = value.m_stencilTestEnabled;

        if (auto object = context.BeginObject())
        {
            object.Field("m_depthCompareOp", compareOp);
            object.Field("m_depthTestEnabled", depthTest);
            object.Field("m_depthWriteEnabled", depthWrite);
            object.Field("m_stencilTestEnabled", stencilTest);
        }

        return context.GetResultCode();
    }


    Serialization::ResultCode DepthStencilState::Deserialize(Serialization::DeserializationContext& context,
                                                             DepthStencilState& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        CompareOp compareOp = value.m_depthCompareOp;
        bool depthTest = value.m_depthTestEnabled;
        bool depthWrite = value.m_depthWriteEnabled;
        bool stencilTest = value.m_stencilTestEnabled;

        if (auto object = context.BeginObject())
        {
            object.Field("m_depthCompareOp", compareOp);
            object.Field("m_depthTestEnabled", depthTest);
            object.Field("m_depthWriteEnabled", depthWrite);
            object.Field("m_stencilTestEnabled", stencilTest);
        }

        const Serialization::ResultCode result = context.GetResultCode();
        if (result == Serialization::ResultCode::kSuccess)
        {
            value.m_depthCompareOp = compareOp;
            value.m_depthTestEnabled = depthTest;
            value.m_depthWriteEnabled = depthWrite;
            value.m_stencilTestEnabled = stencilTest;
        }
        return result;
    }


    Serialization::ResultCode TargetColorBlending::Serialize(Serialization::SerializationContext& context,
                                                             const TargetColorBlending& value)
    {
        static_assert(sizeof(TargetColorBlending) == sizeof(uint32_t));
        if (context.IsBinary())
            return context.RawBytes(value);

        ColorComponentFlags colorWrite = value.m_colorWriteFlags;
        BlendFactor source = value.m_sourceFactor;
        BlendFactor destination = value.m_destinationFactor;
        BlendOperation blendOp = value.m_blendOp;
        BlendFactor sourceAlpha = value.m_sourceAlphaFactor;
        BlendFactor destinationAlpha = value.m_destinationAlphaFactor;
        BlendOperation alphaBlendOp = value.m_alphaBlendOp;
        bool enabled = value.m_blendEnabled;

        if (auto object = context.BeginObject())
        {
            object.Field("m_colorWriteFlags", colorWrite);
            object.Field("m_sourceFactor", source);
            object.Field("m_destinationFactor", destination);
            object.Field("m_blendOp", blendOp);
            object.Field("m_sourceAlphaFactor", sourceAlpha);
            object.Field("m_destinationAlphaFactor", destinationAlpha);
            object.Field("m_alphaBlendOp", alphaBlendOp);
            object.Field("m_blendEnabled", enabled);
        }

        return context.GetResultCode();
    }


    Serialization::ResultCode TargetColorBlending::Deserialize(Serialization::DeserializationContext& context,
                                                               TargetColorBlending& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        ColorComponentFlags colorWrite = value.m_colorWriteFlags;
        BlendFactor source = value.m_sourceFactor;
        BlendFactor destination = value.m_destinationFactor;
        BlendOperation blendOp = value.m_blendOp;
        BlendFactor sourceAlpha = value.m_sourceAlphaFactor;
        BlendFactor destinationAlpha = value.m_destinationAlphaFactor;
        BlendOperation alphaBlendOp = value.m_alphaBlendOp;
        bool enabled = value.m_blendEnabled;

        if (auto object = context.BeginObject())
        {
            object.Field("m_colorWriteFlags", colorWrite);
            object.Field("m_sourceFactor", source);
            object.Field("m_destinationFactor", destination);
            object.Field("m_blendOp", blendOp);
            object.Field("m_sourceAlphaFactor", sourceAlpha);
            object.Field("m_destinationAlphaFactor", destinationAlpha);
            object.Field("m_alphaBlendOp", alphaBlendOp);
            object.Field("m_blendEnabled", enabled);
        }

        const Serialization::ResultCode result = context.GetResultCode();
        if (result == Serialization::ResultCode::kSuccess)
        {
            value.m_colorWriteFlags = colorWrite;
            value.m_sourceFactor = source;
            value.m_destinationFactor = destination;
            value.m_blendOp = blendOp;
            value.m_sourceAlphaFactor = sourceAlpha;
            value.m_destinationAlphaFactor = destinationAlpha;
            value.m_alphaBlendOp = alphaBlendOp;
            value.m_blendEnabled = enabled;
        }
        return result;
    }


    Serialization::ResultCode ColorBlendState::Serialize(Serialization::SerializationContext& context,
                                                         const ColorBlendState& value)
    {
        static_assert(sizeof(ColorBlendState) == sizeof(TargetColorBlending) * Limits::Pipeline::kMaxColorAttachments + 4);
        if (context.IsBinary())
            return context.RawBytes(value);

        if (auto object = context.BeginObject())
        {
            object.Field("m_targetBlendStates", value.m_targetBlendStates);
            object.Field("m_enableIndependentBlend", value.m_enableIndependentBlend);
        }

        return context.GetResultCode();
    }


    Serialization::ResultCode ColorBlendState::Deserialize(Serialization::DeserializationContext& context, ColorBlendState& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        if (auto object = context.BeginObject())
        {
            object.Field("m_targetBlendStates", value.m_targetBlendStates);
            object.Field("m_enableIndependentBlend", value.m_enableIndependentBlend);
        }

        return context.GetResultCode();
    }
} // namespace FE::Graphics::Core
