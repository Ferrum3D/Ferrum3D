#include "Utils.h"

#include <FeCore/IO/IStream.h>

namespace FE
{
    void AssetBuilder::WriteCompactedPages(const festd::span<const std::byte> compressedBuffer, IO::IStream* out)
    {
        // We need to compact pages for some compression methods.

        Memory::BlockReader reader{ compressedBuffer };

        const auto& blockHeader = reader.Read<Compression::BlockHeader>();
        FE_Verify(out->Write(blockHeader));

        auto pageHeader = reader.Read<Compression::PageHeader>();
        for (;;)
        {
            Compression::PageHeader newHeader = pageHeader;
            if (newHeader.m_nextPageOffset != kInvalidIndex)
                newHeader.m_nextPageOffset = pageHeader.m_compressedSize;

            FE_Verify(out->Write(newHeader));

            FE_Verify(out->WriteFromBuffer(reader.m_ptr, pageHeader.m_compressedSize) == pageHeader.m_compressedSize);
            if (pageHeader.m_nextPageOffset == kInvalidIndex)
            {
                reader.m_ptr += pageHeader.m_compressedSize;
                break;
            }

            reader.m_ptr += pageHeader.m_nextPageOffset;
            pageHeader = reader.Read<Compression::PageHeader>();
        }

        FE_Verify(out->Write(reader.Read<Compression::BlockFooter>()));
    }


    AssetBuilder::CompressedBlockWriter::CompressedBlockWriter(IO::IStream* out, const Compression::Compressor* compressor,
                                                               const Crc32 crc)
        : m_crc(crc)
        , m_out(out)
        , m_compressor(compressor)
    {
        m_tempUncompressedBuffer.resize(Compression::kBlockSize);
        m_tempCompressedBuffer.resize(static_cast<uint32_t>(compressor->GetBounds(Compression::kBlockSize)));
    }


    void AssetBuilder::CompressedBlockWriter::WriteBytes(const void* data, const size_t size)
    {
        size_t bytesWritten = 0;

        while (bytesWritten < size)
        {
            const size_t bytesToWrite = Math::Min(size - bytesWritten, Compression::kBlockSize - m_uncompressedDataSize);
            memcpy(m_tempUncompressedBuffer.data() + m_uncompressedDataSize,
                   static_cast<const std::byte*>(data) + bytesWritten,
                   bytesToWrite);
            m_uncompressedDataSize += bytesToWrite;
            bytesWritten += bytesToWrite;

            if (m_uncompressedDataSize == Compression::kBlockSize)
                Flush();
        }
    }


    void AssetBuilder::CompressedBlockWriter::Flush()
    {
        if (m_uncompressedDataSize > 0)
        {
            FE_Verify(m_compressor->Compress(m_crc,
                                             m_tempUncompressedBuffer.data(),
                                             m_tempUncompressedBuffer.size(),
                                             m_tempCompressedBuffer.data(),
                                             m_tempCompressedBuffer.size()));

            WriteCompactedPages(m_tempCompressedBuffer, m_out);
            m_uncompressedDataSize = 0;
        }
    }
} // namespace FE
