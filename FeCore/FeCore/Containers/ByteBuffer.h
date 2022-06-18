#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Containers/IByteBuffer.h>

namespace FE
{
    class ByteBuffer final : public Object<IByteBuffer>
    {
        List<UInt8> m_Data;

    public:
        FE_CLASS_RTTI(ByteBuffer, "96FB77BA-93CF-4321-A41C-1BCB17969B58");

        inline ByteBuffer() = default;
        ByteBuffer(USize size); // NOLINT
        explicit ByteBuffer(const List<UInt8>& data);
        explicit ByteBuffer(List<UInt8>&& data) noexcept;

        [[nodiscard]] UInt8* Data() override;
        [[nodiscard]] const UInt8* Data() const override;
        [[nodiscard]] USize Size() const override;
    };
}
