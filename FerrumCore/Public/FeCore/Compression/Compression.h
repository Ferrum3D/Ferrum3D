#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/Utils/Crc32.h>
#include <festd/base.h>

namespace FE::Compression
{
    constexpr uint32_t kBlockSize = 256 * 1024;
    constexpr uint32_t kGDeflatePageSize = 65536;


    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kInvalidFormat = -1,
        kInsufficientSpace = -2,
        kIntegrityViolation = -3,
        kUnknownError = kDefaultErrorCode<ResultCode>,
    };

    struct DecompressionResult final
    {
        ResultCode m_result = ResultCode::kUnknownError;
        size_t m_decompressedSize = 0;
    };

    enum class Method : uint32_t
    {
        kNone,
        kDeflate,
        kGDeflate,
        kInvalid,
    };


    struct BlockHeader final
    {
        uint32_t m_magic;
        uint32_t m_uncompressedPageSize;
    };


    struct PageHeader final
    {
        uint32_t m_compressedSize;
        uint32_t m_nextPageOffset;
    };


    struct BlockFooter final
    {
        uint32_t m_tailPageUncompressedSize;
        uint32_t m_crc32;
    };


    inline Method DecodeMagic(const uint32_t magic)
    {
        const uint32_t signature = magic & 0x00ffffff;
        if (signature != Math::MakeFourCC('F', 'C', 'B', 0))
            return Method::kInvalid;

        const Method method = static_cast<Method>(magic >> 24);
        if (method >= Method::kInvalid)
            return Method::kInvalid;

        return method;
    }


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

        //! @brief Compress an entire block of data.
        //!
        //! This function will write a block header, followed by one or more pages, followed by a block footer.
        //! Some compression methods may write pages sparsely. It is the caller's responsibility to compact the
        //! resulting data.
        //!
        //! @param crc     The initial CRC32 value. Will be updated and written to the block footer.
        //! @param src     The source data to compress.
        //! @param srcSize The size of the source data in bytes.
        //! @param dst     The buffer to write the compressed data to.
        //! @param dstSize The size of the destination buffer in bytes.
        //!
        //! @return True on success, false on failure.
        [[nodiscard]] bool Compress(Crc32& crc, const void* src, size_t srcSize, void* dst, size_t dstSize) const;

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

        //! @brief Decompress a single page of data.
        //!
        //! Unlike Compressor::Compress, this function does not read any additional data. It is the caller's responsibility to
        //! read any additional data from the source and provide this function with correct parameters.
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
