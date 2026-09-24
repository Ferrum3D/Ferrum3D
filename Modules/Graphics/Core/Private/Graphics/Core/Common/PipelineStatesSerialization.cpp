#include <Graphics/Core/PipelineStates.h>

#include <Core/Serialization/Serialization.h>

namespace FE::Graphics::Core
{
    namespace
    {
        template<class T>
        struct NamedField final
        {
            festd::ascii_view m_name;
            T& m_value;
        };


        template<class T>
        NamedField<T> MakeField(const festd::ascii_view name, T& value)
        {
            return { name, value };
        }


        template<class TContext, class... TFields>
        Serialization::ResultCode TransferObject(TContext& context, TFields... fields)
        {
            if (auto object = context.BeginObject())
                (object.Field(fields.m_name, fields.m_value), ...);
            return context.GetResultCode();
        }
    } // namespace


    Serialization::ResultCode RasterizationState::Serialize(Serialization::SerializationContext& context,
                                                            const RasterizationState& value)
    {
        static_assert(sizeof(RasterizationState) == sizeof(uint32_t));
        if (context.IsBinary())
            return context.RawBytes(value);

        CullingModeFlags cullMode = value.m_cullMode;
        PolygonMode polyMode = value.m_polyMode;
        return TransferObject(context, MakeField("m_cullMode", cullMode), MakeField("m_polyMode", polyMode));
    }


    Serialization::ResultCode RasterizationState::Deserialize(Serialization::DeserializationContext& context,
                                                              RasterizationState& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        CullingModeFlags cullMode = value.m_cullMode;
        PolygonMode polyMode = value.m_polyMode;
        const Serialization::ResultCode result =
            TransferObject(context, MakeField("m_cullMode", cullMode), MakeField("m_polyMode", polyMode));
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
        return TransferObject(context,
                              MakeField("m_depthCompareOp", compareOp),
                              MakeField("m_depthTestEnabled", depthTest),
                              MakeField("m_depthWriteEnabled", depthWrite),
                              MakeField("m_stencilTestEnabled", stencilTest));
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
        const Serialization::ResultCode result = TransferObject(context,
                                                                MakeField("m_depthCompareOp", compareOp),
                                                                MakeField("m_depthTestEnabled", depthTest),
                                                                MakeField("m_depthWriteEnabled", depthWrite),
                                                                MakeField("m_stencilTestEnabled", stencilTest));
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
        return TransferObject(context,
                              MakeField("m_colorWriteFlags", colorWrite),
                              MakeField("m_sourceFactor", source),
                              MakeField("m_destinationFactor", destination),
                              MakeField("m_blendOp", blendOp),
                              MakeField("m_sourceAlphaFactor", sourceAlpha),
                              MakeField("m_destinationAlphaFactor", destinationAlpha),
                              MakeField("m_alphaBlendOp", alphaBlendOp),
                              MakeField("m_blendEnabled", enabled));
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
        const Serialization::ResultCode result = TransferObject(context,
                                                                MakeField("m_colorWriteFlags", colorWrite),
                                                                MakeField("m_sourceFactor", source),
                                                                MakeField("m_destinationFactor", destination),
                                                                MakeField("m_blendOp", blendOp),
                                                                MakeField("m_sourceAlphaFactor", sourceAlpha),
                                                                MakeField("m_destinationAlphaFactor", destinationAlpha),
                                                                MakeField("m_alphaBlendOp", alphaBlendOp),
                                                                MakeField("m_blendEnabled", enabled));
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

        return TransferObject(context,
                              MakeField("m_targetBlendStates", value.m_targetBlendStates),
                              MakeField("m_enableIndependentBlend", value.m_enableIndependentBlend));
    }


    Serialization::ResultCode ColorBlendState::Deserialize(Serialization::DeserializationContext& context, ColorBlendState& value)
    {
        if (context.IsBinary())
            return context.RawBytes(value);

        return TransferObject(context,
                              MakeField("m_targetBlendStates", value.m_targetBlendStates),
                              MakeField("m_enableIndependentBlend", value.m_enableIndependentBlend));
    }
} // namespace FE::Graphics::Core
