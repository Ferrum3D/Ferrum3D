#pragma once
#include <FeCore/Memory/Memory.h>
#include <HAL/ImageFormat.h>
#include <FeCore/Strings/FixedString.h>

namespace FE::Graphics::HAL
{
    struct ShaderInputAttribute
    {
        uint32_t Location;
        Format ElementFormat;
        FixedString<22> ShaderSemantic;
    };


    class ShaderReflection : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(ShaderReflection, "9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~ShaderReflection() override = default;

        virtual festd::span<const ShaderInputAttribute> GetInputAttributes() = 0;
        virtual uint32_t GetInputAttributeLocation(StringSlice semantic) = 0;
    };
} // namespace FE::Graphics::HAL
