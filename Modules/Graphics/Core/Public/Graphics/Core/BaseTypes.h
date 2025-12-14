#pragma once
#include <FeCore/Math/Vector2.h>
#include <FeCore/Math/Vector3UInt.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/Format.h>

namespace FE::Graphics
{
    struct [[nodiscard]] SamplerDescriptor final : public TypedHandle<SamplerDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] TextureSRVDescriptor final : public TypedHandle<TextureSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] TextureUAVDescriptor final : public TypedHandle<TextureUAVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferSRVDescriptor final : public TypedHandle<BufferSRVDescriptor, uint32_t>
    {
    };


    struct [[nodiscard]] BufferUAVDescriptor final : public TypedHandle<BufferUAVDescriptor, uint32_t>
    {
    };

#define FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(name, baseDescriptor)                                                                    \
    template<class T>                                                                                                            \
    using name = baseDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture3DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DArrayDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DArrayDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeArrayDescriptor, TextureSRVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture3DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DArrayDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DArrayDescriptor, TextureUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture3DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DArrayDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DArrayDescriptor, TextureUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(BufferDescriptor, BufferSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(StructuredBufferDescriptor, BufferSRVDescriptor);
    using ByteAddressBufferDescriptor = BufferSRVDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWBufferDescriptor, BufferUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWStructuredBufferDescriptor, BufferUAVDescriptor);
    using RWByteAddressBufferDescriptor = BufferUAVDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentStructuredBufferDescriptor, BufferUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWStructuredBufferDescriptor, BufferUAVDescriptor);
    using GloballyCoherentRWByteAddressBufferDescriptor = BufferUAVDescriptor;

#undef FE_FRAME_GRAPH_ALIAS_DESCRIPTOR
} // namespace FE::Graphics

FE_RTTI_Reflect(FE::Graphics::TextureSRVDescriptor, "4B5172EF-1E7A-43E2-B589-1BAC65428E92");
FE_RTTI_Reflect(FE::Graphics::TextureUAVDescriptor, "39287D51-E573-4428-AADF-74827CD4351E");
FE_RTTI_Reflect(FE::Graphics::BufferSRVDescriptor, "9FFDEB45-4643-43D9-859B-895C5E8012CF");
FE_RTTI_Reflect(FE::Graphics::BufferUAVDescriptor, "B03004C4-3108-46B1-A475-631CDC59BEDD");
FE_RTTI_Reflect(FE::Graphics::SamplerDescriptor, "0ADC65DE-1BA6-4A60-B320-333AD0C2F27B");


namespace FE::Graphics::Core
{
    enum class DescriptorType : uint32_t
    {
        kInvalid,
        kSampler,
        kSRV,
        kUAV,
    };


    struct [[nodiscard]] FrameGraphTextureDescriptorHandle final
    {
        operator TextureSRVDescriptor() const
        {
            return TextureSRVDescriptor{ m_descriptorIndex };
        }

        operator TextureUAVDescriptor() const
        {
            return TextureUAVDescriptor{ m_descriptorIndex };
        }

        uint32_t m_descriptorIndex = kInvalidIndex;
    };


    struct [[nodiscard]] FrameGraphBufferDescriptorHandle final
    {
        operator BufferSRVDescriptor() const
        {
            return BufferSRVDescriptor{ m_descriptorIndex };
        }

        operator BufferUAVDescriptor() const
        {
            return BufferUAVDescriptor{ m_descriptorIndex };
        }

        uint32_t m_descriptorIndex = kInvalidIndex;
    };


    enum class GraphicsAPI : uint32_t
    {
        kNone,
        kVulkan
    };


    enum class PassType : uint32_t
    {
        kGraphics,
        kCompute,
    };


    enum class DeviceQueueType : uint32_t
    {
        kGraphics, //!< Queue that supports graphics, compute, and copy operations.
        kCompute,  //!< Queue that supports compute and copy operations.
        kTransfer, //!< Queue that supports copy operations.

        kCount,
    };


    enum class ShaderSemanticName : uint32_t
    {
        kPosition,
        kColor,
        kNormal,
        kBinormal,
        kTangent,
        kBlendIndices,
        kBlendWeight,
        kTexCoord,
        kCount,
    };


    inline festd::string_view ToString(const ShaderSemanticName name)
    {
        switch (name)
        {
        case ShaderSemanticName::kPosition:
            return "POSITION";
        case ShaderSemanticName::kColor:
            return "COLOR";
        case ShaderSemanticName::kNormal:
            return "NORMAL";
        case ShaderSemanticName::kBinormal:
            return "BINORMAL";
        case ShaderSemanticName::kTangent:
            return "TANGENT";
        case ShaderSemanticName::kBlendIndices:
            return "BLENDINDICES";
        case ShaderSemanticName::kBlendWeight:
            return "BLENDWEIGHT";
        case ShaderSemanticName::kTexCoord:
            return "TEXCOORD";
        case ShaderSemanticName::kCount:
        default:
            FE_DebugBreak();
            return nullptr;
        }
    }


    struct ShaderSemantic final
    {
        ShaderSemanticName m_name : 4;
        uint32_t m_index : 4;

        [[nodiscard]] Env::Name ToName() const;

        static const ShaderSemantic kPosition;
        static const ShaderSemantic kColor;
        static const ShaderSemantic kNormal;
        static const ShaderSemantic kBinormal;
        static const ShaderSemantic kTangent;
        static const ShaderSemantic kBlendIndices;
        static const ShaderSemantic kBlendWeight;
        static const ShaderSemantic kTexCoord;
    };

    inline const ShaderSemantic ShaderSemantic::kPosition{ ShaderSemanticName::kPosition, 0 };
    inline const ShaderSemantic ShaderSemantic::kColor{ ShaderSemanticName::kColor, 0 };
    inline const ShaderSemantic ShaderSemantic::kNormal{ ShaderSemanticName::kNormal, 0 };
    inline const ShaderSemantic ShaderSemantic::kBinormal{ ShaderSemanticName::kBinormal, 0 };
    inline const ShaderSemantic ShaderSemantic::kTangent{ ShaderSemanticName::kTangent, 0 };
    inline const ShaderSemantic ShaderSemantic::kBlendIndices{ ShaderSemanticName::kBlendIndices, 0 };
    inline const ShaderSemantic ShaderSemantic::kBlendWeight{ ShaderSemanticName::kBlendWeight, 0 };
    inline const ShaderSemantic ShaderSemantic::kTexCoord{ ShaderSemanticName::kTexCoord, 0 };


    inline Env::Name ShaderSemantic::ToName() const
    {
        if (m_index == 0)
            return Env::Name{ Fmt::FixedFormatSized<32>("{}", ToString(m_name)) };
        return Env::Name{ Fmt::FixedFormatSized<32>("{}{}", ToString(m_name), m_index) };
    }


    inline uint32_t CalculateMipCount(const Vector3UInt size)
    {
        return 1 + Math::FloorLog2(Math::Max(size.x, Math::Max(size.y, size.z)));
    }


    inline uint32_t CalculateMipCount(const Vector2UInt size)
    {
        return 1 + Math::FloorLog2(Math::Max(size.x, size.y));
    }
} // namespace FE::Graphics::Core
