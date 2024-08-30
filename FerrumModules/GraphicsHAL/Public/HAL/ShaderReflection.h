#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/FixedString.h>
#include <HAL/ImageFormat.h>
#include <HAL/ShaderResourceType.h>
#include <HAL/ShaderStage.h>

namespace FE::Graphics::HAL
{
    struct ShaderInputAttribute final
    {
        uint32_t Location = 0;
        Format ElementFormat = Format::kUndefined;
        FixedString<22> ShaderSemantic;
    };


    struct ShaderResourceBinding final
    {
        FixedString<50> Name;
        uint32_t Stride = 0;
        uint8_t Slot = 0;
        uint8_t Space = 0;
        uint16_t Count = 0;
        ShaderResourceType Type = ShaderResourceType::None;
    };


    class ShaderReflection : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(ShaderReflection, "9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~ShaderReflection() override = default;

        virtual festd::span<const ShaderInputAttribute> GetInputAttributes() const = 0;
        virtual festd::span<const ShaderResourceBinding> GetResourceBindings() const = 0;

        virtual uint32_t GetResourceBindingIndex(StringSlice name) const = 0;
        virtual uint32_t GetInputAttributeLocation(StringSlice semantic) const = 0;
    };
} // namespace FE::Graphics::HAL
