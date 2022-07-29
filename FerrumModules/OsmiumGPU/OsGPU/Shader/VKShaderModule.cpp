#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Shader/VKShaderModule.h>
#include <OsGPU/Shader/VKShaderReflection.h>

namespace FE::Osmium
{
    VKShaderModule::VKShaderModule(VKDevice& dev, const ShaderModuleDesc& desc) // NOLINT(modernize-pass-by-value)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        m_ByteCode.Resize((m_Desc.ByteCode.Length() + 3) / 4);
        memcpy(m_ByteCode.Data(), m_Desc.ByteCode.Data(), m_Desc.ByteCode.Length());

        VkShaderModuleCreateInfo shaderCI{};
        shaderCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCI.codeSize = m_Desc.ByteCode.Length();
        shaderCI.pCode    = m_ByteCode.Data();

        vkCreateShaderModule(m_Device->GetNativeDevice(), &shaderCI, VK_NULL_HANDLE, &m_NativeModule);
    }

    const ShaderModuleDesc& VKShaderModule::GetDesc() const
    {
        return m_Desc;
    }

    VkPipelineShaderStageCreateInfo VKShaderModule::GetStageCI()
    {
        VkPipelineShaderStageCreateInfo info{};
        info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.module = m_NativeModule;
        info.pName  = m_Desc.EntryPoint.Data();
        info.stage  = VKConvert(m_Desc.Stage);
        return info;
    }

    IShaderReflection* VKShaderModule::GetReflection()
    {
        if (m_Reflection == nullptr)
        {
            m_Reflection = MakeShared<VKShaderReflection>(m_ByteCode);
        }

        return m_Reflection.GetRaw();
    }

    FE_VK_OBJECT_DELETER(ShaderModule);

    VKShaderModule::~VKShaderModule()
    {
        FE_DELETE_VK_OBJECT(ShaderModule, m_NativeModule);
    }
} // namespace FE::Osmium
