#pragma once
#include <FeCore/Containers/SmallVector.h>
#include <HAL/ShaderReflection.h>
#include <HAL/Vulkan/Common/Config.h>
#include <spirv_hlsl.hpp>

namespace FE::Graphics::Vulkan
{
    class ShaderReflection : public HAL::ShaderReflection
    {
        festd::small_vector<HAL::ShaderInputAttribute> m_InputAttributes;
        festd::small_vector<HAL::ShaderResourceBinding> m_ResourceBindings;

        void ParseInputAttributes(const spirv_cross::CompilerHLSL* pCompiler,
                                  const spirv_cross::ShaderResources& shaderResources);
        void ParseResourceBindings(const spirv_cross::CompilerHLSL* pCompiler,
                                   const spirv_cross::ShaderResources& shaderResources);

    public:
        FE_RTTI_Class(ShaderReflection, "686E6EBE-8038-4E26-919C-70834410BC1F");

        explicit ShaderReflection(festd::span<uint32_t> byteCode);

        festd::span<const HAL::ShaderInputAttribute> GetInputAttributes() const override;
        festd::span<const HAL::ShaderResourceBinding> GetResourceBindings() const override;

        uint32_t GetResourceBindingIndex(StringSlice name) const override;
        uint32_t GetInputAttributeLocation(StringSlice semantic) const override;
    };

    FE_ENABLE_IMPL_CAST(ShaderReflection);
} // namespace FE::Graphics::Vulkan
