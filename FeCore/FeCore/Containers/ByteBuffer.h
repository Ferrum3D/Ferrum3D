#pragma once
#include <FeCore/Containers/IByteBuffer.h>
#include <FeCore/Containers/List.h>

namespace FE
{
    //! \brief A buffer that stores contiguous memory as an array of bytes. Implementation of IByteBuffer.
    class ByteBuffer final : public Object<IByteBuffer>
    {
        List<UInt8> m_Data;

    public:
        FE_CLASS_RTTI(ByteBuffer, "96FB77BA-93CF-4321-A41C-1BCB17969B58");

        inline ByteBuffer() = default;

        //! \brief Create a ByteBuffer with specified size.
        ByteBuffer(USize size); // NOLINT

        //! \brief Create from a list and copy it.
        //!
        //! \param [in] data - The list that stores the data.
        explicit ByteBuffer(const List<UInt8>& data);

        //! \brief Create from a list and move it.
        //!
        //! \param [in] data - The list that stores the data.
        explicit ByteBuffer(List<UInt8>&& data) noexcept;

        [[nodiscard]] UInt8* Data() override;
        [[nodiscard]] const UInt8* Data() const override;
        [[nodiscard]] USize Size() const override;
    };
} // namespace FE
