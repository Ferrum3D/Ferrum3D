#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/ShaderModule.h>
#include <Graphics/RHI/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    ShaderModule::ShaderModule(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode ShaderModule::Init(const RHI::ShaderModuleDesc& desc)
    {
        m_desc = desc;

        m_byteCode.resize(Math::CeilDivide(m_desc.m_byteCode.size(), 4));
        memcpy(m_byteCode.data(), m_desc.m_byteCode.data(), m_desc.m_byteCode.size());

        VkShaderModuleCreateInfo shaderCI{};
        shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCI.codeSize = m_desc.m_byteCode.size();
        shaderCI.pCode = m_byteCode.data();

        vkCreateShaderModule(NativeCast(m_device), &shaderCI, VK_NULL_HANDLE, &m_nativeModule);
        return RHI::ResultCode::kSuccess;
    }


    const RHI::ShaderModuleDesc& ShaderModule::GetDesc() const
    {
        return m_desc;
    }


    VkPipelineShaderStageCreateInfo ShaderModule::GetStageCI()
    {
        VkPipelineShaderStageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.module = m_nativeModule;
        info.pName = m_desc.m_entryPoint.c_str();
        info.stage = VKConvert(m_desc.m_stage);
        return info;
    }


    RHI::ShaderReflection* ShaderModule::GetReflection()
    {
        if (m_reflection == nullptr)
            m_reflection = Rc<ShaderReflection>::DefaultNew(m_byteCode);

        return m_reflection.Get();
    }


    ShaderModule::~ShaderModule()
    {
        if (m_nativeModule)
            vkDestroyShaderModule(NativeCast(m_device), m_nativeModule, nullptr);
    }
} // namespace FE::Graphics::Vulkan
