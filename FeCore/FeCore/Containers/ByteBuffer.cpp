#include <FeCore/Containers/ByteBuffer.h>

namespace FE
{
    ByteBuffer::ByteBuffer(USize size) // NOLINT
    {
        m_Data.Resize(size);
    }

    ByteBuffer::ByteBuffer(const List<UInt8>& data)
        : m_Data(data)
    {
    }

    ByteBuffer::ByteBuffer(List<UInt8>&& data) noexcept
        : m_Data(std::move(data))
    {
    }

    UInt8* ByteBuffer::Data()
    {
        return m_Data.Data();
    }

    const UInt8* ByteBuffer::Data() const
    {
        return m_Data.Data();
    }

    USize ByteBuffer::Size() const
    {
        return m_Data.Size();
    }
}
