#pragma once
#include <Core/Base/BaseTypes.h>
#include <Core/RTTI/RTTI.h>

namespace FE::Compression
{
    constexpr uint32_t kBlockSize = 256 * 1024;


    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kInvalidFormat = -1,
        kInsufficientSpace = -2,
        kUnknownError = kDefaultErrorCode<ResultCode>,
    };

    struct CompressionResult final
    {
        ResultCode m_result = ResultCode::kUnknownError;
        size_t m_compressedSize = 0;
    };

    struct DecompressionResult final
    {
        ResultCode m_result = ResultCode::kUnknownError;
        size_t m_decompressedSize = 0;
    };

    enum class Method : uint32_t
    {
        kNone,

        kDeflate FE_META(DisplayName = Deflate),
        kZstd FE_META(DisplayName = Zstd),
        kInvalid FE_META(DisplayName = Invalid),
    };


    struct Compressor final
    {
        Compressor() = default;
        ~Compressor();

        Compressor(const Compressor&) = delete;
        Compressor& operator=(const Compressor&) = delete;

        Compressor(Compressor&& other) noexcept
        {
            std::swap(m_method, other.m_method);
            std::swap(m_level, other.m_level);
            std::swap(m_impl, other.m_impl);
        }

        Compressor& operator=(Compressor&& other) noexcept
        {
            std::swap(m_method, other.m_method);
            std::swap(m_level, other.m_level);
            std::swap(m_impl, other.m_impl);
            return *this;
        }

        void Reset();

        [[nodiscard]] size_t GetBounds(size_t uncompressedSize) const;

        //! @brief Compress data into the provided buffer.
        //!
        //! @param src     The source data to compress.
        //! @param srcSize The size of the source data in bytes.
        //! @param dst     The buffer to write the compressed data to.
        //! @param dstSize The size of the destination buffer in bytes.
        //!
        //! @return The result and the number of compressed bytes written.
        [[nodiscard]] CompressionResult Compress(const void* src, size_t srcSize, void* dst, size_t dstSize) const;

        static Compressor Create(Method method, int32_t level = 6);

    private:
        Compressor(const Method method, const int32_t level, void* impl)
            : m_method(method)
            , m_level(level)
            , m_impl(impl)
        {
        }

        Method m_method = Method::kNone;
        int32_t m_level = 0;
        void* m_impl = nullptr;
    };


    struct Decompressor final
    {
        Decompressor() = default;
        ~Decompressor();

        Decompressor(const Decompressor&) = delete;
        Decompressor& operator=(const Decompressor&) = delete;

        Decompressor(Decompressor&& other) noexcept
        {
            std::swap(m_method, other.m_method);
            std::swap(m_impl, other.m_impl);
        }

        Decompressor& operator=(Decompressor&& other) noexcept
        {
            std::swap(m_method, other.m_method);
            std::swap(m_impl, other.m_impl);
            return *this;
        }

        void Reset();

        //! @brief Decompress data into the provided buffer.
        //!
        //! @param src     The source data to decompress.
        //! @param srcSize The size of the source data in bytes.
        //! @param dst     The buffer to write the decompressed data to.
        //! @param dstSize The size of the destination buffer in bytes.
        //!
        //! @return The result of the decompression operation.
        [[nodiscard]] DecompressionResult Decompress(const void* src, size_t srcSize, void* dst, size_t dstSize) const;

        static Decompressor Create(Method method);

    private:
        Decompressor(const Method method, void* impl)
            : m_method(method)
            , m_impl(impl)
        {
        }

        Method m_method = Method::kNone;
        void* m_impl = nullptr;
    };
} // namespace FE::Compression

FE_RTTI_Reflect(FE::Compression::Method, "80AD0E9B-049D-41EC-B8E5-AB32C15DDCFA");
