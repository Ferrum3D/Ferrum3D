#pragma once
#include <FeCore/Strings/Format.h>
#include <Graphics/Core/Base/BaseTypes.h>

namespace FE::Graphics::Core
{
    enum class ImageAspect : uint32_t
    {
        kColor,
        kDepth,
        kStencil,
        kDepthStencil,
    };


    enum class FormatType : uint32_t
    {
        kNone,
        kInt,
        kFloat,
        kNorm,
        kCount,
    };


    enum class FormatChannelCount : uint32_t
    {
        k1,
        k2,
        k3,
        k4,
        kCount,
    };


    namespace Internal
    {
        constexpr uint32_t MakeFormatEnumValue(const FormatType type, const uint32_t byteSize,
                                               const FormatChannelCount channelCount, const ImageAspect aspectFlags,
                                               const bool blockCompressed, const bool isSigned, const bool isSRGB,
                                               const uint32_t index)
        {
            // clang-format off
            return (festd::to_underlying(type)             << 30)
                 | (byteSize                               << 25)
                 | (festd::to_underlying(channelCount)     << 23)
                 | (festd::to_underlying(aspectFlags)      << 20)
                 | ((blockCompressed           ? 1 : 0)    << 19)
                 | ((isSigned                  ? 1 : 0)    << 18)
                 | ((isSRGB                    ? 1 : 0)    << 17)
                 | index;
            // clang-format on
        }
    } // namespace Internal


#define FE_MAKE_FORMAT(type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                                         \
    Internal::MakeFormatEnumValue(FormatType::k##type,                                                                           \
                                  static_cast<uint32_t>(byteSize),                                                               \
                                  FormatChannelCount::k##channelCount,                                                           \
                                  ImageAspect::k##aspectFlags,                                                                   \
                                  static_cast<bool>(bc),                                                                         \
                                  static_cast<bool>(sign),                                                                       \
                                  static_cast<bool>(srgb),                                                                       \
                                  index)


    //    name                         type  byteSize     channelCount aspectFlags bc  signed srgb index
    // clang-format off
#define FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, bits, chan, baseIndex)                                                \
    _Func(baseName## _SINT,              Int, bits*chan/8, chan,        Color,      0,  1,     0,   baseIndex + 0)               \
    _Func(baseName## _UINT,              Int, bits*chan/8, chan,        Color,      0,  0,     0,   baseIndex + 1)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_2(_Func, baseName, bits, chan, baseIndex)                                                \
    _Func(baseName## _SNORM,            Norm, bits*chan/8, chan,        Color,      0,  1,     0,   baseIndex + 0)               \
    _Func(baseName## _UNORM,            Norm, bits*chan/8, chan,        Color,      0,  0,     0,   baseIndex + 1)
    // clang-format on

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_8(_Func, baseName, chan, baseIndex)                                                      \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 8, chan, baseIndex)                                                       \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_2(_Func, baseName, 8, chan, baseIndex + 2)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_16(_Func, baseName, chan, baseIndex)                                                     \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 16, chan, baseIndex)                                                      \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_2(_Func, baseName, 16, chan, baseIndex + 2)                                                  \
    _Func(baseName##_SFLOAT, Float, 2 * chan, chan, Color, 0, 1, 0, baseIndex + 4)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_32(_Func, baseName, chan, baseIndex)                                                     \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 32, chan, baseIndex)                                                      \
    _Func(baseName##_SFLOAT, Float, 4 * chan, chan, Color, 0, 1, 0, baseIndex + 2)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_64(_Func, baseName, chan, baseIndex)                                                     \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 64, chan, baseIndex)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, baseName, bits, chan, baseIndex)                                                  \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_##bits(_Func, baseName, chan, baseIndex)

    // clang-format off
#define FE_EXPAND_BASIC_FORMATS(_Func)                                                                                           \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R8,              8,      1,         10)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R16,            16,      1,         20)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32,            32,      1,         30)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64,            64,      1,         40)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R8G8,            8,      2,         50)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R16G16,         16,      2,         60)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32G32,         32,      2,         70)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64G64,         64,      2,         80)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32G32B32,      32,      3,         90)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64G64B64,      64,      3,        100)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R8G8B8A8,        8,      4,        110)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R16G16B16A16,   16,      4,        120)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32G32B32A32,   32,      4,        130)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64G64B64A64,   64,      4,        140)                                               \
    /*    name                         type  byteSize     channelCount aspectFlags bc  signed srgb index       */                \
    _Func(R8_SRGB,                     Norm, 1,           1,           Color,      0,  0,     1,  150           )                \
    _Func(R8G8_SRGB,                   Norm, 2,           1,           Color,      0,  0,     1,  151           )                \
    _Func(R8G8B8_SRGB,                 Norm, 3,           1,           Color,      0,  0,     1,  152           )                \
    _Func(R8G8B8A8_SRGB,               Norm, 4,           1,           Color,      0,  0,     1,  153           )                \
    _Func(B8G8R8A8_SRGB,               Norm, 4,           1,           Color,      0,  0,     1,  154           )                \
    _Func(B8G8R8A8_UNORM,              Norm, 4,           1,           Color,      0,  0,     0,  155           )

#define FE_EXPAND_BC_FORMATS(_Func)                                                                                              \
    _Func(BC1_UNORM,                   Norm, 8,           4,           Color,      1,  0,     0,  200           )                \
    _Func(BC1_SRGB,                    Norm, 8,           4,           Color,      1,  0,     1,  201           )                \
    _Func(BC2_UNORM,                   Norm, 16,          4,           Color,      1,  0,     0,  202           )                \
    _Func(BC2_SRGB,                    Norm, 16,          4,           Color,      1,  0,     1,  203           )                \
    _Func(BC3_UNORM,                   Norm, 16,          4,           Color,      1,  0,     0,  204           )                \
    _Func(BC3_SRGB,                    Norm, 16,          4,           Color,      1,  0,     1,  205           )                \
    _Func(BC4_UNORM,                   Norm, 8,           1,           Color,      1,  0,     0,  206           )                \
    _Func(BC4_SNORM,                   Norm, 8,           1,           Color,      1,  1,     0,  207           )                \
    _Func(BC5_UNORM,                   Norm, 16,          2,           Color,      1,  0,     0,  208           )                \
    _Func(BC5_SNORM,                   Norm, 16,          2,           Color,      1,  1,     0,  209           )                \
    _Func(BC6H_UFLOAT,                Float, 16,          3,           Color,      1,  0,     0,  210           )                \
    _Func(BC6H_SFLOAT,                Float, 16,          3,           Color,      1,  1,     0,  211           )                \
    _Func(BC7_UNORM,                   Norm, 16,          4,           Color,      1,  0,     0,  212           )                \
    _Func(BC7_SRGB,                    Norm, 16,          4,           Color,      1,  0,     1,  213           )

#define FE_EXPAND_PACK_FORMATS(_Func)                                                                                            \
    _Func(A2R10G10B10_UINT,             Int, 4,           4,           Color,      0,  0,     0,  300           )                \
    _Func(A2R10G10B10_UNORM,           Norm, 4,           4,           Color,      0,  0,     0,  301           )                \
    _Func(B10G11R11_UFLOAT,           Float, 4,           3,           Color,      0,  0,     0,  302           )

#define FE_EXPAND_DS_FORMATS(_Func)                                                                                              \
    _Func(D16_UNORM,                   Norm, 2,           1,           Depth,      0,  0,     0,  400           )                \
    _Func(D32_SFLOAT,                 Float, 4,           1,           Depth,      0,  1,     0,  401           )                \
    _Func(D24_UNORM_S8_UINT,           None, 4,           2,    DepthStencil,      0,  0,     0,  402           )                \
    _Func(D32_SFLOAT_S8_UINT,          None, 8,           2,    DepthStencil,      0,  1,     0,  403           )
    // clang-format on

#define FE_EXPAND_FORMATS(_Func)                                                                                                 \
    FE_EXPAND_BASIC_FORMATS(_Func)                                                                                               \
    FE_EXPAND_BC_FORMATS(_Func)                                                                                                  \
    FE_EXPAND_PACK_FORMATS(_Func)                                                                                                \
    FE_EXPAND_DS_FORMATS(_Func)


    enum class Format : uint32_t
    {
        kUndefined = 0,

#define FE_DECL_FORMAT_ENUM_VALUE(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                        \
    k##name = FE_MAKE_FORMAT(type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index),

        FE_EXPAND_FORMATS(FE_DECL_FORMAT_ENUM_VALUE)

#undef FE_DECL_FORMAT_ENUM_VALUE
    };

    // static_assert((festd::to_underlying(Format::kR32G32B32A32_UINT) == FE_MAKE_FORMAT(Int, 16, 4, Color, 0, 0, 0, 43)));


    // clang-format off
#define FE_EXPAND_VERTEX_CHANNEL_FORMATS(_Func)                                                                                  \
    _Func(R8_SINT)                                                                                                               \
    _Func(R8_UINT)                                                                                                               \
    _Func(R8_SNORM)                                                                                                              \
    _Func(R8_UNORM)                                                                                                              \
    _Func(R8G8_SINT)                                                                                                             \
    _Func(R8G8_UINT)                                                                                                             \
    _Func(R8G8_SNORM)                                                                                                            \
    _Func(R8G8_UNORM)                                                                                                            \
    _Func(R8G8B8A8_SINT)                                                                                                         \
    _Func(R8G8B8A8_UINT)                                                                                                         \
    _Func(R8G8B8A8_SNORM)                                                                                                        \
    _Func(R8G8B8A8_UNORM)                                                                                                        \
    _Func(R16_SINT)                                                                                                              \
    _Func(R16_UINT)                                                                                                              \
    _Func(R16_SNORM)                                                                                                             \
    _Func(R16_UNORM)                                                                                                             \
    _Func(R16_SFLOAT)                                                                                                            \
    _Func(R16G16_SINT)                                                                                                           \
    _Func(R16G16_UINT)                                                                                                           \
    _Func(R16G16_SNORM)                                                                                                          \
    _Func(R16G16_UNORM)                                                                                                          \
    _Func(R16G16_SFLOAT)                                                                                                         \
    _Func(R16G16B16A16_SINT)                                                                                                     \
    _Func(R16G16B16A16_UINT)                                                                                                     \
    _Func(R16G16B16A16_SNORM)                                                                                                    \
    _Func(R16G16B16A16_UNORM)                                                                                                    \
    _Func(R16G16B16A16_SFLOAT)                                                                                                   \
    _Func(R32_SINT)                                                                                                              \
    _Func(R32_UINT)                                                                                                              \
    _Func(R32_SFLOAT)                                                                                                            \
    _Func(R32G32_SINT)                                                                                                           \
    _Func(R32G32_UINT)                                                                                                           \
    _Func(R32G32_SFLOAT)                                                                                                         \
    _Func(R32G32B32_SINT)                                                                                                        \
    _Func(R32G32B32_UINT)                                                                                                        \
    _Func(R32G32B32_SFLOAT)                                                                                                      \
    _Func(R32G32B32A32_SINT)                                                                                                     \
    _Func(R32G32B32A32_UINT)                                                                                                     \
    _Func(R32G32B32A32_SFLOAT)                                                                                                   \
    _Func(A2R10G10B10_UINT)                                                                                                      \
    _Func(A2R10G10B10_UNORM)                                                                                                     \
    _Func(B10G11R11_UFLOAT)
    // clang-format on

    enum class VertexChannelFormat : uint32_t
    {
        kUndefined,

#define FE_DECL_VERTEX_CHANNEL_FORMAT_ENUM_VALUE(name) k##name,

        FE_EXPAND_VERTEX_CHANNEL_FORMATS(FE_DECL_VERTEX_CHANNEL_FORMAT_ENUM_VALUE)

#undef FE_DECL_VERTEX_CHANNEL_FORMAT_ENUM_VALUE
    };


    inline VertexChannelFormat TranslateFormat(const Format format)
    {
        switch (format)
        {
        case Format::kUndefined:
            return VertexChannelFormat::kUndefined;

#define FE_TRANSLATE_FORMAT_TO_VERTEX_CHANNEL_FORMAT_CASE(name)                                                                  \
    case Format::k##name:                                                                                                        \
        return VertexChannelFormat::k##name;

            FE_EXPAND_VERTEX_CHANNEL_FORMATS(FE_TRANSLATE_FORMAT_TO_VERTEX_CHANNEL_FORMAT_CASE)

#undef FE_TRANSLATE_FORMAT_TO_VERTEX_CHANNEL_FORMAT_CASE

        default:
            // The specified format is not supported as a vertex channel format.
            FE_DebugBreak();
            return VertexChannelFormat::kUndefined;
        }
    }


    inline Format TranslateFormat(const VertexChannelFormat format)
    {
        switch (format)
        {
        case VertexChannelFormat::kUndefined:
            return Format::kUndefined;

#define FE_TRANSLATE_VERTEX_CHANNEL_FORMAT_TO_FORMAT_CASE(name)                                                                  \
    case VertexChannelFormat::k##name:                                                                                           \
        return Format::k##name;

            FE_EXPAND_VERTEX_CHANNEL_FORMATS(FE_TRANSLATE_VERTEX_CHANNEL_FORMAT_TO_FORMAT_CASE)

#undef FE_TRANSLATE_VERTEX_CHANNEL_FORMAT_TO_FORMAT_CASE

        default:
            FE_DebugBreak();
            return Format::kUndefined;
        }
    }


    union FormatInfo final
    {
        FormatInfo()
            : m_format(Format::kUndefined)
        {
        }

        explicit FormatInfo(const Format format)
            : m_format(format)
        {
        }

        explicit FormatInfo(const VertexChannelFormat format)
            : m_format(TranslateFormat(format))
        {
        }

        [[nodiscard]] uint32_t GetChannelCount() const
        {
            return festd::to_underlying(m_channelCount) + 1;
        }

        [[nodiscard]] uint32_t CalculateRowPitch(const uint32_t width, const uint32_t mipIndex = 0) const
        {
            const uint32_t blockSize = m_isBlockCompressed ? 4 : 1;
            const uint32_t blockCountW = Math::Max(Math::CeilDivide(width >> mipIndex, blockSize), 1u);
            return blockCountW * m_texelByteSize;
        }

        [[nodiscard]] uint32_t CalculateMipByteSize(const Vector3UInt size, const uint32_t mipIndex = 0) const
        {
            const uint32_t blockSize = m_isBlockCompressed ? 4 : 1;
            const uint32_t blockCountX = Math::Max(Math::CeilDivide(size.x >> mipIndex, blockSize), 1u);
            const uint32_t blockCountY = Math::Max(Math::CeilDivide(size.y >> mipIndex, blockSize), 1u);
            const uint32_t blockCountZ = Math::Max(size.z >> mipIndex, 1u);
            return blockCountX * blockCountY * blockCountZ * m_texelByteSize;
        }

        [[nodiscard]] uint32_t CalculateMipByteSize(const Vector2UInt size, const uint32_t mipIndex = 0) const
        {
            const uint32_t blockSize = m_isBlockCompressed ? 4 : 1;
            const uint32_t blockCountX = Math::Max(Math::CeilDivide(size.x >> mipIndex, blockSize), 1u);
            const uint32_t blockCountY = Math::Max(Math::CeilDivide(size.y >> mipIndex, blockSize), 1u);
            return blockCountX * blockCountY * m_texelByteSize;
        }

        [[nodiscard]] uint32_t CalculateImageByteSize(const Vector3UInt size, const uint32_t mipCount) const
        {
            uint32_t totalSize = 0;
            for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
            {
                totalSize += CalculateMipByteSize(size, mipCount);
            }

            return totalSize;
        }

        [[nodiscard]] uint32_t CalculateImageByteSize(const Vector2UInt size, const uint32_t mipCount) const
        {
            uint32_t totalSize = 0;
            for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
            {
                totalSize += CalculateMipByteSize(size, mipCount);
            }

            return totalSize;
        }

        [[nodiscard]] uint32_t CalculateImageByteSize(const Vector3UInt size) const
        {
            return CalculateImageByteSize(size, CalculateMipCount(size));
        }

        [[nodiscard]] uint32_t CalculateImageByteSize(const Vector2UInt size) const
        {
            return CalculateImageByteSize(size, CalculateMipCount(size));
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_format != Format::kUndefined;
        }

        struct
        {
            uint32_t m_formatIndex : 17;
            uint32_t m_isSRGB : 1;
            uint32_t m_isSigned : 1;
            uint32_t m_isBlockCompressed : 1;
            ImageAspect m_aspectFlags : 3;
            FormatChannelCount m_channelCount : 2;
            uint32_t m_texelByteSize : 5;
            FormatType m_type : 2;
        };

        Format m_format;
    };

    static_assert(sizeof(Format) == sizeof(FormatInfo));


    inline festd::string_view ToString(const Format format)
    {
        switch (format)
        {
#define FE_DECL_FORMAT_NAME(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                              \
    case Format::k##name:                                                                                                        \
        return #name;

            FE_EXPAND_FORMATS(FE_DECL_FORMAT_NAME)

        case Format::kUndefined:
            return "<undefined>";

        default:
            return "<unknown>";
        }
    }


    inline uint32_t GetFormatSize(const Format format)
    {
        return FormatInfo{ format }.m_texelByteSize;
    }

    inline uint32_t GetFormatSize(const VertexChannelFormat format)
    {
        return FormatInfo{ format }.m_texelByteSize;
    }
} // namespace FE::Graphics::Core


template<class TBuffer>
struct FE::Fmt::ValueFormatter<TBuffer, FE::Graphics::Core::Format>
{
    void Format(TBuffer& buffer, const FE::Graphics::Core::Format& value) const
    {
        buffer += FE::Graphics::Core::ToString(value);
    }
}; // namespace FE::Fmt
