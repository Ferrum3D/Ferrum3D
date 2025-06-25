#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/ShaderResourceType.h>
#include <Graphics/Core/ShaderSpecialization.h>

namespace FE::Graphics::Core
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
        uint32_t m_slot : 16;
        uint32_t m_space : 16;
        uint32_t m_count;
        ShaderResourceType m_type = ShaderResourceType::kNone;
    };


    struct ShaderRootConstant final
    {
        Env::Name m_name;
        uint32_t m_offset : 16;
        uint32_t m_byteSize : 16;
    };


    struct ShaderReflection : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ShaderReflection, "9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~ShaderReflection() override = default;

        virtual festd::span<const ShaderInputAttribute> GetInputAttributes() const = 0;
        virtual festd::span<const ShaderResourceBinding> GetResourceBindings() const = 0;
        virtual festd::span<const ShaderRootConstant> GetRootConstants() const = 0;
        virtual festd::span<const Env::Name> GetSpecializationConstantNames() const = 0;

        virtual uint32_t GetResourceBindingIndex(Env::Name name) const = 0;
        virtual uint32_t GetInputAttributeLocation(Env::Name semantic) const = 0;
    };
} // namespace FE::Graphics::Core
