#pragma once
#include <FeCore/Strings/Format.h>
#include <FeCore/Strings/String.h>
#include <HAL/Common/BaseTypes.h>

namespace FE::Graphics::HAL
{
    enum class ImageAspect : uint8_t
    {
        kColor,
        kDepth,
        kStencil
    };


    enum class ImageAspectFlags : uint8_t
    {
        kNone,
        kColor = 1 << enum_cast(ImageAspect::kColor),
        kDepth = 1 << enum_cast(ImageAspect::kDepth),
        kStencil = 1 << enum_cast(ImageAspect::kStencil),
        kDepthStencil = kDepth | kStencil,
        kAll = kDepth | kStencil | kColor
    };

    FE_ENUM_OPERATORS(ImageAspectFlags);


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
        inline constexpr uint32_t MakeFormatEnumValue(FormatType type, uint32_t byteSize, FormatChannelCount channelCount,
                                                      ImageAspectFlags aspectFlags, bool blockCompressed, bool isSigned,
                                                      bool isSRGB, uint32_t index)
        {
            // clang-format off
            return enum_cast(type)                        << 30
                 | byteSize                               << 25
                 | enum_cast(channelCount)                << 23
                 | enum_cast(aspectFlags)                 << 20
                 | (blockCompressed           ? 1 : 0)    << 19
                 | (isSigned                  ? 1 : 0)    << 18
                 | (isSRGB                    ? 1 : 0)    << 17
                 | index;
            // clang-format on
        }
    } // namespace Internal


#define FE_MAKE_FORMAT(type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                                         \
    Internal::MakeFormatEnumValue(FormatType::k##type,                                                                           \
                                  static_cast<uint32_t>(byteSize),                                                               \
                                  FormatChannelCount::k##channelCount,                                                           \
                                  ImageAspectFlags::k##aspectFlags,                                                              \
                                  static_cast<bool>(bc),                                                                         \
                                  static_cast<bool>(sign),                                                                       \
                                  static_cast<bool>(srgb),                                                                       \
                                  index)


    //    name                         type  byteSize     channelCount aspectFlags bc  signed srgb index
    // clang-format off
#define FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, bits, chan, baseIndex)                                                \
    _Func(baseName##_SINT,                Int, bits*chan/8, chan,        Color,      0,  1,     0,   baseIndex + 0)              \
    _Func(baseName##_UINT,                Int, bits*chan/8, chan,        Color,      0,  0,     0,   baseIndex + 1)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_2(_Func, baseName, bits, chan, baseIndex)                                                \
    _Func(baseName##_SNORM,              Norm, bits*chan/8, chan,        Color,      0,  1,     0,   baseIndex + 0)              \
    _Func(baseName##_UNORM,              Norm, bits*chan/8, chan,        Color,      0,  0,     0,   baseIndex + 1)
    // clang-format on

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_8(_Func, baseName, chan, baseIndex)                                                      \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 8, chan, baseIndex)                                                       \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_2(_Func, baseName, 8, chan, baseIndex + 2)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_16(_Func, baseName, chan, baseIndex)                                                     \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 16, chan, baseIndex)                                                      \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_2(_Func, baseName, 16, chan, baseIndex + 2)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_32(_Func, baseName, chan, baseIndex)                                                     \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 32, chan, baseIndex)                                                      \
    _Func(baseName##_SFLOAT, Float, 4 * chan, chan, Color, 0, 1, 0, baseIndex + 2)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK_64(_Func, baseName, chan, baseIndex)                                                     \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_1(_Func, baseName, 64, chan, baseIndex)

#define FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, baseName, bits, chan, baseIndex)                                                  \
    FE_BASIC_FORMAT_EXPANSION_BLOCK_##bits(_Func, baseName, chan, baseIndex)

    // clang-format off
#define FE_EXPAND_BASIC_FORMATS(_Func)                                                                                           \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R8,             8,       1,         10)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R16,            16,      1,         15)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32,            32,      1,         20)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64,            64,      1,         25)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R8G8,           8,       2,         30)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R16G16,         16,      2,         35)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32G32,         32,      2,         40)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64G64,         64,      2,         45)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32G32B32,      32,      3,         50)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64G64B64,      64,      3,         55)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R8G8B8A8,       8,       4,         60)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R16G16B16A16,   16,      4,         65)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R32G32B32A32,   32,      4,         70)                                               \
    FE_BASIC_FORMAT_EXPANSION_BLOCK(_Func, R64G64B64A64,   64,      4,         75)                                               \
    /*    name                         type  byteSize     channelCount aspectFlags bc  signed srgb index       */                \
    _Func(R8_SRGB,                     Norm, 1,           1,           Color,      0,  0,     1,   14           )                \
    _Func(R8G8_SRGB,                   Norm, 2,           1,           Color,      0,  0,     1,   34           )                \
    _Func(R8G8B8_SRGB,                 Norm, 3,           1,           Color,      0,  0,     1,   54           )                \
    _Func(R8G8B8A8_SRGB,               Norm, 4,           1,           Color,      0,  0,     1,   64           )                \
    _Func(B8G8R8A8_SRGB,               Norm, 4,           1,           Color,      0,  0,     1,   74           )

#define FE_EXPAND_BC_FORMATS(_Func)                                                                                              \
    _Func(BC1_UNORM,                   Norm, 8,           4,           Color,      1,  0,     0,   80           )                \
    _Func(BC1_SRGB,                    Norm, 8,           4,           Color,      1,  0,     1,   81           )                \
    _Func(BC2_UNORM,                   Norm, 16,          4,           Color,      1,  0,     0,   82           )                \
    _Func(BC2_SRGB,                    Norm, 16,          4,           Color,      1,  0,     1,   83           )                \
    _Func(BC3_UNORM,                   Norm, 16,          4,           Color,      1,  0,     0,   84           )                \
    _Func(BC3_SRGB,                    Norm, 16,          4,           Color,      1,  0,     1,   85           )                \
    _Func(BC4_UNORM,                   Norm, 8,           1,           Color,      1,  0,     0,   86           )                \
    _Func(BC4_SNORM,                   Norm, 8,           1,           Color,      1,  1,     0,   87           )                \
    _Func(BC5_UNORM,                   Norm, 16,          2,           Color,      1,  0,     0,   88           )                \
    _Func(BC5_SNORM,                   Norm, 16,          2,           Color,      1,  1,     0,   89           )                \
    _Func(BC6H_UFLOAT,                Float, 16,          3,           Color,      1,  0,     0,   90           )                \
    _Func(BC6H_SFLOAT,                Float, 16,          3,           Color,      1,  1,     0,   91           )                \
    _Func(BC7_UNORM,                   Norm, 16,          4,           Color,      1,  0,     0,   92           )                \
    _Func(BC7_SRGB,                    Norm, 16,          4,           Color,      1,  0,     1,   93           )

#define FE_EXPAND_PACK_FORMATS(_Func)                                                                                            \
    _Func(R11G11B10_SFLOAT,           Float, 4,           3,           Color,      0,  1,     0,   94           )

#define FE_EXPAND_DS_FORMATS(_Func)                                                                                              \
    _Func(D16_UNORM,                   Norm, 2,           1,           Depth,      0,  0,     0,   95           )                \
    _Func(D32_SFLOAT,                 Float, 4,           1,           Depth,      0,  1,     0,   96           )                \
    _Func(D24_UNORM_S8_UINT,           None, 4,           2,    DepthStencil,      0,  0,     0,   97           )                \
    _Func(D32_SFLOAT_S8_UINT,          None, 8,           2,    DepthStencil,      0,  1,     0,   98           )
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

    static_assert((enum_cast(Format::kR32G32B32A32_UINT) == FE_MAKE_FORMAT(Int, 16, 4, Color, 0, 0, 0, 71)));


    struct FormatInfo final
    {
        inline FormatInfo() = default;

        inline explicit FormatInfo(Format format)
        {
            static_assert(sizeof(Format) == sizeof(FormatInfo));
            memcpy(this, &format, sizeof(Format));
        }

        [[nodiscard]] inline Format GetFormat() const
        {
            return bit_cast<Format>(*this);
        }

        [[nodiscard]] inline FormatType GetType() const
        {
            return static_cast<FormatType>(m_Type);
        }

        [[nodiscard]] inline uint32_t GetByteSize() const
        {
            return m_ByteSize;
        }

        [[nodiscard]] inline uint32_t GetChannelCount() const
        {
            return m_ChannelCount + 1;
        }

        [[nodiscard]] inline ImageAspectFlags GetAspectFlags() const
        {
            return static_cast<ImageAspectFlags>(m_AspectFlags);
        }

        [[nodiscard]] inline bool IsBlockCompressed() const
        {
            return m_IsBlockCompressed;
        }

        [[nodiscard]] inline bool IsSigned() const
        {
            return m_IsSigned;
        }

        [[nodiscard]] inline bool IsSRGB() const
        {
            return m_IsSRGB;
        }

        [[nodiscard]] inline uint32_t GetFormatID() const
        {
            return m_FormatIndex;
        }

        [[nodiscard]] inline uint32_t CalculateRowPitch(uint32_t width, uint32_t mipIndex = 0) const
        {
            const uint32_t blockSize = m_IsBlockCompressed ? 4 : 1;
            const uint32_t blockCountW = std::max(Math::CeilDivide(width >> mipIndex, blockSize), 1u);
            return blockCountW * m_ByteSize;
        }

        [[nodiscard]] inline uint32_t CalculateMipByteSize(Size size, uint32_t mipIndex = 0) const
        {
            const uint32_t blockSize = m_IsBlockCompressed ? 4 : 1;
            const uint32_t blockCountW = std::max(Math::CeilDivide(size.Width >> mipIndex, blockSize), 1u);
            const uint32_t blockCountH = std::max(Math::CeilDivide(size.Height >> mipIndex, blockSize), 1u);
            return std::max(size.Depth >> mipIndex, 1u) * blockCountW * blockCountH * m_ByteSize;
        }

        [[nodiscard]] inline uint32_t CalculateImageByteSize(Size size, uint32_t mipCount) const
        {
            uint32_t totalSize = 0;
            for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
            {
                totalSize += CalculateMipByteSize(size, mipCount);
            }

            return totalSize;
        }

        [[nodiscard]] inline uint32_t CalculateImageByteSize(Size size) const
        {
            return CalculateImageByteSize(size, CalculateMipCount(size));
        }

    private:
        uint32_t m_Type : 2;
        uint32_t m_ByteSize : 5;
        uint32_t m_ChannelCount : 2;
        uint32_t m_AspectFlags : 3;
        uint32_t m_IsBlockCompressed : 1;
        uint32_t m_IsSigned : 1;
        uint32_t m_IsSRGB : 1;
        uint32_t m_FormatIndex : 17;
    };


    inline StringSlice ToString(Format format)
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

    inline uint32_t GetFormatSize(Format format)
    {
        return FormatInfo{ format }.GetByteSize();
    }
} // namespace FE::Graphics::HAL

namespace FE::Fmt
{
    template<class TBuffer>
    struct ValueFormatter<TBuffer, FE::Graphics::HAL::Format>
    {
        void Format(TBuffer& buffer, const FE::Graphics::HAL::Format& value) const
        {
            buffer.Append(FE::Graphics::HAL::ToString(value));
        }
    };
} // namespace FE::Fmt
