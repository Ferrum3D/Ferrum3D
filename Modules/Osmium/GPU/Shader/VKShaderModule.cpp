#include <GPU/Device/VKDevice.h>
#include <GPU/Shader/VKShaderModule.h>
#include <GPU/Shader/VKShaderReflection.h>

namespace FE::GPU
{
    VKShaderModule::VKShaderModule(VKDevice& dev, const ShaderModuleDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        m_ByteCode.resize((m_Desc.ByteCodeSize + 3) / 4);
        memcpy(m_ByteCode.data(), m_Desc.ByteCode, m_Desc.ByteCodeSize);

        vk::ShaderModuleCreateInfo shaderCI{};
        shaderCI.codeSize = m_Desc.ByteCodeSize;
        shaderCI.pCode    = m_ByteCode.data();

        auto nativeDev = m_Device->GetNativeDevice();
        m_NativeModule = nativeDev.createShaderModuleUnique(shaderCI);
    }

    const ShaderModuleDesc& VKShaderModule::GetDesc() const
    {
        return m_Desc;
    }

    vk::PipelineShaderStageCreateInfo VKShaderModule::GetStageCI()
    {
        vk::PipelineShaderStageCreateInfo info{};
        info.module = m_NativeModule.get();
        info.pName = m_Desc.EntryPoint.Data();
        info.stage = VKConvert(m_Desc.Stage);
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
} // namespace FE::GPU
