#pragma once
#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/Format.h>
#include <Graphics/RHI/Base/Limits.h>

namespace FE::Graphics::RHI
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


    enum class MemoryType : uint32_t
    {
        kDeviceLocal,
        kHostVisible,
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


    inline StringSlice ToString(const ShaderSemanticName name)
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
    };


    inline Env::Name ShaderSemantic::ToName() const
    {
        return Env::Name{ Fmt::FixedFormatSized<32>("{}{}", ToString(m_name), m_index) };
    }


    struct Offset final
    {
        int32_t x : Limits::Image::kMaxMipCount + 1;
        int32_t y : Limits::Image::kMaxMipCount + 1;
        int32_t z : Limits::Image::kMaxMipCount + 1;

        constexpr Offset()
            : x(0)
            , y(0)
            , z(0)
        {
        }

        constexpr explicit Offset(const int32_t x)
            : x(x)
            , y(0)
            , z(0)
        {
        }

        constexpr Offset(const int32_t x, const int32_t y)
            : x(x)
            , y(y)
            , z(0)
        {
        }

        constexpr Offset(const int32_t x, const int32_t y, const int32_t z)
            : x(x)
            , y(y)
            , z(z)
        {
        }
    };


    struct Size final
    {
        uint32_t width : Limits::Image::kMaxMipCount + 1;
        uint32_t height : Limits::Image::kMaxMipCount + 1;
        uint32_t depth : Limits::Image::kMaxMipCount + 1;

        constexpr Size()
            : width(0)
            , height(0)
            , depth(1)
        {
        }

        constexpr explicit Size(const uint32_t w)
            : width(w)
            , height(0)
            , depth(1)
        {
        }

        constexpr Size(const uint32_t w, const uint32_t h)
            : width(w)
            , height(h)
            , depth(1)
        {
        }

        constexpr Size(const uint32_t w, const uint32_t h, const uint32_t d)
            : width(w)
            , height(h)
            , depth(d)
        {
        }
    };


    inline uint32_t CalculateMipCount(const Size size)
    {
        return 1 + Math::FloorLog2(Math::Max(size.width, Math::Max(size.height, size.depth)));
    }
} // namespace FE::Graphics::RHI


template<>
struct eastl::hash<FE::Graphics::RHI::Size>
{
    size_t operator()(const FE::Graphics::RHI::Size& size) const noexcept
    {
        return FE::HashAll(size.width, size.height, size.depth);
    }
};
