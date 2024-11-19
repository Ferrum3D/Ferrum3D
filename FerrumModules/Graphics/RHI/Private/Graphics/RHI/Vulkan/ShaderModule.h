#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/RHI/ShaderModule.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderReflection;

    inline VkShaderStageFlags VKConvert(RHI::ShaderStageFlags source)
    {
        auto result = 0;
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & RHI::ShaderStageFlags::k##ferrum) != RHI::ShaderStageFlags::kNone)                                             \
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


    inline VkShaderStageFlagBits VKConvert(RHI::ShaderStage source)
    {
        switch (source)
        {
        case RHI::ShaderStage::kVertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case RHI::ShaderStage::kPixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case RHI::ShaderStage::kHull:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case RHI::ShaderStage::kDomain:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case RHI::ShaderStage::kGeometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case RHI::ShaderStage::kCompute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            FE_AssertMsg(false, "Invalid ShaderStage");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }


    struct ShaderModule final : public RHI::ShaderModule
    {
        FE_RTTI_Class(ShaderModule, "823A44B8-72BD-4F19-BCFA-32D077B06B3A");

        ShaderModule(RHI::Device* device);
        ~ShaderModule() override;

        [[nodiscard]] VkShaderModule GetNative() const
        {
            return m_nativeModule;
        }

        RHI::ResultCode Init(const RHI::ShaderModuleDesc& desc) override;

        [[nodiscard]] const RHI::ShaderModuleDesc& GetDesc() const override;

        VkPipelineShaderStageCreateInfo GetStageCI();
        RHI::ShaderReflection* GetReflection() override;

    private:
        festd::vector<uint32_t> m_byteCode;
        RHI::ShaderModuleDesc m_desc;
        VkShaderModule m_nativeModule = VK_NULL_HANDLE;
        Rc<ShaderReflection> m_reflection;
    };

    FE_ENABLE_NATIVE_CAST(ShaderModule);
} // namespace FE::Graphics::Vulkan
