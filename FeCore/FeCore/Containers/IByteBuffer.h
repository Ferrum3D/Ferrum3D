#pragma once
#include <FeCore/Memory/Object.h>

namespace FE
{
    class IByteBuffer : public IObject
    {
    public:
        FE_CLASS_RTTI(IByteBuffer, "C3D7541E-361C-4B5B-929B-48CECC1E335D");

        ~IByteBuffer() override = default;

        [[nodiscard]] virtual UInt8* Data() = 0;
        [[nodiscard]] virtual const UInt8* Data() const = 0;
        [[nodiscard]] virtual USize Size() const = 0;
    };

}
