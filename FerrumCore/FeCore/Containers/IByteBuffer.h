#pragma once
#include <FeCore/Memory/Object.h>

namespace FE
{
    //! \brief A ByteBuffer interface.
    //!
    //! A buffer that stores contiguous memory as an array of bytes.
    class IByteBuffer : public IObject
    {
    public:
        FE_CLASS_RTTI(IByteBuffer, "C3D7541E-361C-4B5B-929B-48CECC1E335D");

        ~IByteBuffer() override = default;

        //! \brief Get a pointer to the beginning of buffer storage.
        [[nodiscard]] virtual UInt8* Data() = 0;

        //! \brief Get a pointer to the beginning of buffer storage.
        [[nodiscard]] virtual const UInt8* Data() const = 0;

        //! \brief Get the size of buffer in bytes.
        [[nodiscard]] virtual USize Size() const = 0;

        //! \brief Copy data from this buffer to dest.
        virtual void CopyTo(IByteBuffer* dest) const = 0;
    };
} // namespace FE
