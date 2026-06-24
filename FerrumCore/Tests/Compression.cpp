#include <Core/Compression/Compression.h>

#include <Core/Math/Random.h>
#include <festd/vector.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    festd::vector<std::byte> MakeTestData(const uint32_t size)
    {
        DefaultRandom random;
        festd::vector<std::byte> result(size);
        for (uint32_t index = 0; index < size; ++index)
            result[index] = static_cast<std::byte>(random.RandUInt64() & 0xff);

        return result;
    }
} // namespace


TEST(Compression, RawRoundTrip)
{
    const festd::vector<std::byte> source = MakeTestData(Compression::kBlockSize + 8192);

    constexpr Compression::Method kCompressionMethods[]{ Compression::Method::kNone,
                                                         Compression::Method::kDeflate,
                                                         Compression::Method::kZstd };

    for (const Compression::Method method : kCompressionMethods)
    {
        auto compressor = Compression::Compressor::Create(method);
        festd::vector<std::byte> compressed(static_cast<uint32_t>(compressor.GetBounds(source.size())));

        const Compression::CompressionResult compressionResult =
            compressor.Compress(source.data(), source.size(), compressed.data(), compressed.size());
        ASSERT_EQ(compressionResult.m_result, Compression::ResultCode::kSuccess);
        ASSERT_LE(compressionResult.m_compressedSize, compressed.size());

        if (method == Compression::Method::kNone)
        {
            EXPECT_EQ(compressionResult.m_compressedSize, source.size());
            EXPECT_EQ(memcmp(source.data(), compressed.data(), source.size()), 0);
        }

        auto decompressor = Compression::Decompressor::Create(method);
        festd::vector<std::byte> decompressed(source.size());
        const Compression::DecompressionResult decompressionResult = decompressor.Decompress(compressed.data(),
                                                                                             compressionResult.m_compressedSize,
                                                                                             decompressed.data(),
                                                                                             decompressed.size());

        ASSERT_EQ(decompressionResult.m_result, Compression::ResultCode::kSuccess);
        ASSERT_EQ(decompressionResult.m_decompressedSize, source.size());
        EXPECT_EQ(decompressed, source);
    }
}


TEST(Compression, InsufficientDestination)
{
    const festd::vector<std::byte> source = MakeTestData(4096);
    festd::array<std::byte, 1> destination{};

    constexpr Compression::Method kCompressionMethods[]{ Compression::Method::kNone,
                                                         Compression::Method::kDeflate,
                                                         Compression::Method::kZstd };

    for (const Compression::Method method : kCompressionMethods)
    {
        auto compressor = Compression::Compressor::Create(method);
        const Compression::CompressionResult result =
            compressor.Compress(source.data(), source.size(), destination.data(), destination.size());

        EXPECT_EQ(result.m_result, Compression::ResultCode::kInsufficientSpace);
        EXPECT_EQ(result.m_compressedSize, 0);
    }
}


TEST(Compression, InvalidCompressedData)
{
    const festd::array<std::byte, 32> invalidData{};
    festd::array<std::byte, 1024> destination{};

    constexpr Compression::Method kCompressionMethods[]{ Compression::Method::kDeflate, Compression::Method::kZstd };
    for (const Compression::Method method : kCompressionMethods)
    {
        auto decompressor = Compression::Decompressor::Create(method);
        const Compression::DecompressionResult result =
            decompressor.Decompress(invalidData.data(), invalidData.size(), destination.data(), destination.size());

        EXPECT_EQ(result.m_result, Compression::ResultCode::kInvalidFormat);
        EXPECT_EQ(result.m_decompressedSize, 0);
    }
}


TEST(Compression, ContextsAreReusable)
{
    const festd::vector<std::byte> source = MakeTestData(16384);

    constexpr Compression::Method kCompressionMethods[]{ Compression::Method::kDeflate, Compression::Method::kZstd };
    for (const Compression::Method method : kCompressionMethods)
    {
        size_t compressedSize = 0;
        festd::vector<std::byte> compressed;

        {
            auto compressor = Compression::Compressor::Create(method);
            compressed.resize(static_cast<uint32_t>(compressor.GetBounds(source.size())));
            const Compression::CompressionResult result =
                compressor.Compress(source.data(), source.size(), compressed.data(), compressed.size());
            ASSERT_EQ(result.m_result, Compression::ResultCode::kSuccess);
            compressedSize = result.m_compressedSize;
        }

        auto decompressor = Compression::Decompressor::Create(method);
        festd::vector<std::byte> decompressed(source.size());
        const Compression::DecompressionResult result =
            decompressor.Decompress(compressed.data(), compressedSize, decompressed.data(), decompressed.size());

        ASSERT_EQ(result.m_result, Compression::ResultCode::kSuccess);
        EXPECT_EQ(decompressed, source);
    }
}
