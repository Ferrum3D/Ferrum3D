#include <Core/IO/MemoryStream.h>

namespace FE::IO
{
    size_t WriteOnlyMemoryStream::ReadToBuffer(void*, size_t)
    {
        return 0;
    }


    size_t WriteOnlyMemoryStream::WriteFromBuffer(const void* buffer, const size_t byteSize)
    {
        FE_Assert(byteSize <= kPageCapacity);

        uint32_t pageIndex = kInvalidIndex;
        if (!m_pages.empty())
            pageIndex = m_pages.size() - 1;

        if (pageIndex == kInvalidIndex || m_pages[pageIndex].m_byteSize + byteSize > kPageCapacity)
        {
            Page& page = m_pages.emplace_back();
            page.m_byteSize = 0;
            page.m_data = static_cast<std::byte*>(Memory::AllocateVirtual(kPageCapacity));
            ++pageIndex;
        }

        Page& page = m_pages[pageIndex];
        memcpy(page.m_data + page.m_byteSize, buffer, byteSize);
        page.m_byteSize += byteSize;

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
