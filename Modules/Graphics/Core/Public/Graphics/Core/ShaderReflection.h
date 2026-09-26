#pragma once
#include <Core/Memory/Memory.h>
#include <Graphics/Core/Format.h>
#include <Graphics/Core/ShaderResourceType.h>
#include <Graphics/Core/ShaderSpecialization.h>
#include <festd/vector.h>

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
        uint32_t m_offset : 16 = 0;
        uint32_t m_byteSize : 16 = 0;
    };


    enum class ShaderStructMemberType : uint32_t
    {
        kInvalid,
        kFloat,
        kInt,
        kUint,

        kTexture1DDescriptor,
        kTexture2DDescriptor,
        kTexture3DDescriptor,
        kTexture1DArrayDescriptor,
        kTexture2DArrayDescriptor,
        kTextureCubeDescriptor,
        kTextureCubeArrayDescriptor,
    };


    struct ShaderStructMember final
    {
        Env::Name m_name;
        uint32_t m_offset = 0;
        uint32_t m_byteSize = 0;
        ShaderStructMemberType m_type : 16 = ShaderStructMemberType::kInvalid;
        uint32_t m_vectorSize : 16 = 0;

        bool operator==(const ShaderStructMember&) const = default;
    };


    struct ShaderStructLayout final
    {
        Env::Name m_name;
        uint32_t m_byteSize = 0;
        festd::vector<ShaderStructMember> m_members;
    };


    struct ShaderReflection : public Memory::RefCountedObjectBase
    {
        FE_RTTI("9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~ShaderReflection() override = default;

        virtual festd::span<const ShaderInputAttribute> GetInputAttributes() const = 0;
        virtual festd::span<const ShaderResourceBinding> GetResourceBindings() const = 0;
        virtual festd::span<const ShaderRootConstant> GetRootConstants() const = 0;
        virtual festd::span<const ShaderStructLayout> GetStructLayouts() const = 0;
        virtual festd::span<const Env::Name> GetSpecializationConstantNames() const = 0;

        virtual uint32_t GetResourceBindingIndex(Env::Name name) const = 0;
        virtual uint32_t GetInputAttributeLocation(Env::Name semantic) const = 0;
    };
} // namespace FE::Graphics::Core
