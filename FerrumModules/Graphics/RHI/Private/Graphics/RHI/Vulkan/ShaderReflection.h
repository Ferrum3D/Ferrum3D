#pragma once
#include <FeCore/Containers/SmallVector.h>
#include <Graphics/RHI/ShaderReflection.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>
#include <spirv_hlsl.hpp>

namespace FE::Graphics::Vulkan
{
    struct ShaderReflection final : public RHI::ShaderReflection
    {
        FE_RTTI_Class(ShaderReflection, "686E6EBE-8038-4E26-919C-70834410BC1F");

        explicit ShaderReflection(festd::span<uint32_t> byteCode);

        festd::span<const RHI::ShaderInputAttribute> GetInputAttributes() const override;
        festd::span<const RHI::ShaderResourceBinding> GetResourceBindings() const override;

        uint32_t GetResourceBindingIndex(Env::Name name) const override;
        uint32_t GetInputAttributeLocation(Env::Name semantic) const override;

    private:
        festd::small_vector<RHI::ShaderInputAttribute> m_inputAttributes;
        festd::small_vector<RHI::ShaderResourceBinding> m_resourceBindings;

        void ParseInputAttributes(const spirv_cross::CompilerHLSL* pCompiler,
                                  const spirv_cross::ShaderResources& shaderResources);
        void ParseResourceBindings(const spirv_cross::CompilerHLSL* pCompiler,
                                   const spirv_cross::ShaderResources& shaderResources);
    };

    FE_ENABLE_IMPL_CAST(ShaderReflection);
} // namespace FE::Graphics::Vulkan
