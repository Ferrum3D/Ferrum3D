#pragma once
#include <FeCore/Memory/Memory.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Shader/IShaderModule.h>

namespace FE::Osmium
{
    class VKShaderReflection;

    inline VkShaderStageFlags VKConvert(ShaderStageFlags source)
    {
        auto result = 0;
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & ShaderStageFlags::ferrum) != ShaderStageFlags::None)                                                           \
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

    inline VkShaderStageFlagBits VKConvert(ShaderStage source)
    {
        switch (source)
        {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Pixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Hull:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStage::Domain:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            FE_UNREACHABLE("Invalid ShaderStage");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    class VKDevice;

    class VKShaderModule : public IShaderModule
    {
        eastl::vector<uint32_t> m_ByteCode;
        ShaderModuleDesc m_Desc;
        VkShaderModule m_NativeModule;
        VKDevice* m_Device;
        Rc<VKShaderReflection> m_Reflection;

    public:
        FE_RTTI_Class(VKShaderModule, "823A44B8-72BD-4F19-BCFA-32D077B06B3A");

        VKShaderModule(VKDevice& dev, const ShaderModuleDesc& desc);
        ~VKShaderModule() override;

        [[nodiscard]] const ShaderModuleDesc& GetDesc() const override;

        VkPipelineShaderStageCreateInfo GetStageCI();
        IShaderReflection* GetReflection() override;
    };
} // namespace FE::Osmium
