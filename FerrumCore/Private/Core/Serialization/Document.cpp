#include <Core/Serialization/Document.h>

namespace FE::Serialization
{
    Format DetectDocumentFormat(IO::IStream& stream, const uint64_t expectedPackedHeader)
    {
        const uintptr_t position = stream.Tell();
        uint64_t header = 0;
        const bool headerRead = stream.Read(header);
        stream.Seek(static_cast<intptr_t>(position), IO::SeekMode::kBegin);
        if (headerRead && header == expectedPackedHeader)
            return Format::kPackedBinary;

        if (static_cast<uint32_t>(header) == UINT32_C(0x47545346))
            return Format::kTaggedBinary;

        char first = 0;
        while (stream.Read(first))
        {
            if (first != ' ' && first != '\t' && first != '\r' && first != '\n')
                break;
        }

        stream.Seek(static_cast<intptr_t>(position), IO::SeekMode::kBegin);
        return first == '{' ? Format::kJson : Format::kPackedBinary;
    }
} // namespace FE::Serialization
