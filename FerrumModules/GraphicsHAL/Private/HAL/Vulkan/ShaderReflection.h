#pragma once
#include <HAL/ShaderReflection.h>
#include <HAL/Vulkan/Common/Config.h>
#include <spirv_hlsl.hpp>

namespace FE::Graphics::Vulkan
{
    class ShaderReflection : public HAL::ShaderReflection
    {
        eastl::vector<HAL::ShaderInputAttribute> m_InputAttributes;

        void ParseInputAttributes(spirv_cross::CompilerHLSL& compiler);

    public:
        FE_RTTI_Class(ShaderReflection, "686E6EBE-8038-4E26-919C-70834410BC1F");

        explicit ShaderReflection(const eastl::vector<uint32_t>& byteCode);
        festd::span<const HAL::ShaderInputAttribute> GetInputAttributes() override;
        uint32_t GetInputAttributeLocation(StringSlice semantic) override;
    };

    FE_ENABLE_IMPL_CAST(ShaderReflection);
} // namespace FE::Graphics::Vulkan
