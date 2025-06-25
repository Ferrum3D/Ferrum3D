#pragma once
#include <Graphics/Core/ShaderReflection.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/vector.h>
#include <spirv_hlsl.hpp>

namespace FE::Graphics::Vulkan
{
    struct ShaderReflection final : public Core::ShaderReflection
    {
        FE_RTTI_Class(ShaderReflection, "686E6EBE-8038-4E26-919C-70834410BC1F");

        explicit ShaderReflection(festd::span<const uint32_t> byteCode);

        festd::span<const Core::ShaderInputAttribute> GetInputAttributes() const override;
        festd::span<const Core::ShaderResourceBinding> GetResourceBindings() const override;
        festd::span<const Core::ShaderRootConstant> GetRootConstants() const override;
        festd::span<const Env::Name> GetSpecializationConstantNames() const override;

        uint32_t GetResourceBindingIndex(Env::Name name) const override;
        uint32_t GetInputAttributeLocation(Env::Name semantic) const override;

    private:
        festd::inline_vector<Core::ShaderInputAttribute> m_inputAttributes;
        festd::inline_vector<Core::ShaderResourceBinding> m_resourceBindings;
        festd::inline_vector<Core::ShaderRootConstant> m_rootConstants;
        festd::inline_vector<Env::Name> m_specializationConstantNames;

        void ParseInputAttributes(const spirv_cross::CompilerHLSL* compiler,
                                  const spirv_cross::ShaderResources& shaderResources);
        void ParseResourceBindings(const spirv_cross::CompilerHLSL* compiler,
                                   const spirv_cross::ShaderResources& shaderResources);
        void ParseSpecializationConstants(const spirv_cross::CompilerHLSL* compiler);
    };

    FE_ENABLE_IMPL_CAST(ShaderReflection);
} // namespace FE::Graphics::Vulkan
