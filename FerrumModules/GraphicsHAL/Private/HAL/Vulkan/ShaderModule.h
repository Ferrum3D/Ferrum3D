#pragma once
#include <FeCore/Memory/Memory.h>
#include <HAL/ShaderModule.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class ShaderReflection;

    inline VkShaderStageFlags VKConvert(HAL::ShaderStageFlags source)
    {
        auto result = 0;
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & HAL::ShaderStageFlags::ferrum) != HAL::ShaderStageFlags::None)                                                 \
    result |= VK_SHADER_STAGE_##vulkan##_BIT

        FE_CVT_ENTRY(Pixel, FRAGMENT);
        FE_CVT_ENTRY(Vertex, VERTEX);
        FE_CVT_ENTRY(Hull, TESSELLATION_CONTROL);
        FE_CVT_ENTRY(Domain, TESSELLATION_EVALUATION);
        FE_CVT_ENTRY(Geometry, GEOMETRY);
        FE_CVT_ENTRY(Compute, COMPUTE);
#undef FE_CVT_ENTRY

        return result;
    }


    inline VkShaderStageFlagBits VKConvert(HAL::ShaderStage source)
    {
        switch (source)
        {
        case HAL::ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case HAL::ShaderStage::Pixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case HAL::ShaderStage::Hull:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case HAL::ShaderStage::Domain:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case HAL::ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case HAL::ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            FE_UNREACHABLE("Invalid ShaderStage");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }


    class ShaderModule : public HAL::ShaderModule
    {
        eastl::vector<uint32_t> m_ByteCode;
        HAL::ShaderModuleDesc m_Desc;
        VkShaderModule m_NativeModule = VK_NULL_HANDLE;
        Rc<ShaderReflection> m_Reflection;

    public:
        FE_RTTI_Class(ShaderModule, "823A44B8-72BD-4F19-BCFA-32D077B06B3A");

        ShaderModule(HAL::Device* pDevice);
        ~ShaderModule() override;

        [[nodiscard]] inline VkShaderModule GetNative() const
        {
            return m_NativeModule;
        }

        HAL::ResultCode Init(const HAL::ShaderModuleDesc& desc) override;

        [[nodiscard]] const HAL::ShaderModuleDesc& GetDesc() const override;

        VkPipelineShaderStageCreateInfo GetStageCI();
        HAL::ShaderReflection* GetReflection() override;
    };

    FE_ENABLE_IMPL_CAST(ShaderModule);
} // namespace FE::Graphics::Vulkan
