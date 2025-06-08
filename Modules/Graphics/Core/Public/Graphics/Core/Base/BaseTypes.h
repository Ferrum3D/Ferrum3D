#pragma once
#include <FeCore/Math/Vector3UInt.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/Format.h>

namespace FE::Graphics::Core
{
    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kUnknownError = kDefaultErrorCode<ResultCode>,
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


    enum class HardwareQueueKindFlags : uint32_t
    {
        kNone = 0, //!< Invalid or unspecified value.

        kGraphicsBit = 1 << 0, //!< Queue that supports graphics operations.
        kComputeBit = 1 << 1,  //!< Queue that supports compute operations.
        kTransferBit = 1 << 2, //!< Queue that supports copy operations.

        //! @brief Queue for graphics + compute + copy operations.
        kGraphics = kGraphicsBit | kComputeBit | kTransferBit,
        //! @brief Queue for compute + copy operations.
        kCompute = kComputeBit | kTransferBit,
        //! @brief Queue for copy operations.
        kTransfer = kTransferBit,
    };

    FE_ENUM_OPERATORS(HardwareQueueKindFlags);


    enum class BindFlags : uint32_t
    {
        kNone = 0,
        kVertexBuffer = 1 << 0,
        kIndexBuffer = 1 << 1,
        kConstantBuffer = 1 << 2,
        kShaderResource = 1 << 3,
        kStreamOutput = 1 << 4,
        kRenderTarget = 1 << 5,
        kDepthStencil = 1 << 6,
        kUnorderedAccess = 1 << 7,
        kIndirectDrawArgs = 1 << 8,
        kInputAttachment = 1 << 9,
    };

    FE_ENUM_OPERATORS(BindFlags);


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
} // namespace FE::Graphics::Core
