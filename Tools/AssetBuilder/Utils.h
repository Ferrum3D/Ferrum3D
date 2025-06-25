#pragma once
#include <FeCore/Compression/Compression.h>
#include <FeCore/IO/IStream.h>
#include <festd/span.h>
#include <festd/vector.h>

namespace FE::AssetBuilder
{
    void WriteCompactedPages(festd::span<const std::byte> compressedBuffer, IO::IStream* out);


    struct CompressedBlockWriter final
    {
        CompressedBlockWriter(IO::IStream* out, const Compression::Compressor* compressor, Crc32 crc);

        CompressedBlockWriter(IO::IStream* out, const Compression::Compressor* compressor)
            : CompressedBlockWriter(out, compressor, {})
        {
        }

        void WriteBytes(const void* data, size_t size);

        template<class T>
        void Write(const T& value)
        {
            WriteBytes(&value, sizeof(T));
        }

        void Flush();

        Crc32 m_crc;
        IO::IStream* m_out;
        const Compression::Compressor* m_compressor;

        size_t m_uncompressedDataSize = 0;
        festd::vector<std::byte> m_tempUncompressedBuffer;
        festd::vector<std::byte> m_tempCompressedBuffer;
    };
} // namespace FE::AssetBuilder
