#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/FixedString.h>
#include <Graphics/RHI/ImageFormat.h>
#include <Graphics/RHI/ShaderResourceType.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct ShaderInputAttribute final
    {
        uint32_t m_location = 0;
        Format m_elementFormat = Format::kUndefined;
        Env::Name m_shaderSemantic;
    };


    struct ShaderResourceBinding final
    {
        Env::Name m_name;
        uint32_t m_stride;
        uint32_t m_slot : 8;
        uint32_t m_space : 8;
        uint32_t m_count : 16;
        ShaderResourceType m_type = ShaderResourceType::kNone;
    };


    struct ShaderReflection : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ShaderReflection, "9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~ShaderReflection() override = default;

        virtual festd::span<const ShaderInputAttribute> GetInputAttributes() const = 0;
        virtual festd::span<const ShaderResourceBinding> GetResourceBindings() const = 0;

        virtual uint32_t GetResourceBindingIndex(Env::Name name) const = 0;
        virtual uint32_t GetInputAttributeLocation(Env::Name semantic) const = 0;
    };
} // namespace FE::Graphics::RHI
