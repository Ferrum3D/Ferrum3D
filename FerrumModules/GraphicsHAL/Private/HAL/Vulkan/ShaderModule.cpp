#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/ShaderModule.h>
#include <HAL/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    ShaderModule::ShaderModule(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode ShaderModule::Init(const HAL::ShaderModuleDesc& desc)
    {
        m_Desc = desc;

        m_ByteCode.resize((m_Desc.ByteCode.size() + 3) / 4);
        memcpy(m_ByteCode.data(), m_Desc.ByteCode.data(), m_Desc.ByteCode.size());

        VkShaderModuleCreateInfo shaderCI{};
        shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCI.codeSize = m_Desc.ByteCode.size();
        shaderCI.pCode = m_ByteCode.data();

        vkCreateShaderModule(ImplCast(m_pDevice)->GetNativeDevice(), &shaderCI, VK_NULL_HANDLE, &m_NativeModule);
        return HAL::ResultCode::Success;
    }


    const HAL::ShaderModuleDesc& ShaderModule::GetDesc() const
    {
        return m_Desc;
    }


    VkPipelineShaderStageCreateInfo ShaderModule::GetStageCI()
    {
        VkPipelineShaderStageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.module = m_NativeModule;
        info.pName = m_Desc.EntryPoint.Data();
        info.stage = VKConvert(m_Desc.Stage);
        return info;
    }


    HAL::ShaderReflection* ShaderModule::GetReflection()
    {
        if (m_Reflection == nullptr)
            m_Reflection = Rc<ShaderReflection>::DefaultNew(m_ByteCode);

        return m_Reflection.Get();
    }


    ShaderModule::~ShaderModule()
    {
        if (m_NativeModule)
            vkDestroyShaderModule(ImplCast(m_pDevice)->GetNativeDevice(), m_NativeModule, nullptr);
    }
} // namespace FE::Graphics::Vulkan
