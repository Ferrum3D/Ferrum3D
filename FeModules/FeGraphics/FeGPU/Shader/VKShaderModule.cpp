#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Shader/VKShaderModule.h>

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
} // namespace FE::GPU
