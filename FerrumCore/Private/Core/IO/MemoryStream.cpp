#include <Core/IO/MemoryStream.h>

namespace FE::IO
{
    WriteOnlyMemoryStream::~WriteOnlyMemoryStream()
    {
        for (const Page& page : m_pages)
            Memory::FreeVirtual(page.m_data, kPageCapacity);
    }


    size_t WriteOnlyMemoryStream::ReadToBuffer(void*, size_t)
    {
        return 0;
    }


    size_t WriteOnlyMemoryStream::WriteFromBuffer(const void* buffer, const size_t byteSize)
    {
        const auto* source = static_cast<const std::byte*>(buffer);
        size_t remaining = byteSize;
        while (remaining > 0)
        {
            if (m_pages.empty() || m_pages.back().m_byteSize == kPageCapacity)
            {
                Page& page = m_pages.emplace_back();
                page.m_byteSize = 0;
                page.m_data = static_cast<std::byte*>(Memory::AllocateVirtual(kPageCapacity));
            }

            Page& page = m_pages.back();
            const size_t copySize = Math::Min(remaining, kPageCapacity - page.m_byteSize);
            memcpy(page.m_data + page.m_byteSize, source, copySize);
            page.m_byteSize += copySize;
            source += copySize;
            remaining -= copySize;
        }

        m_totalByteSize += byteSize;

        return byteSize;
    }


    void WriteOnlyMemoryStream::DumpAll(festd::pmr::vector<std::byte>& output)
    {
        FE_Assert(m_totalByteSize <= Constants::kMaxU32);
        output.resize(static_cast<uint32_t>(m_totalByteSize));

        uintptr_t position = 0;
        for (const Page page : m_pages)
        {
            memcpy(output.data() + position, page.m_data, page.m_byteSize);
            position += page.m_byteSize;
        }
    }
} // namespace FE::IO
