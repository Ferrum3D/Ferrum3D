#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Shader/IShaderModule.h>

namespace FE::GPU
{
    class VKDevice;

    class VKShaderModule : public Object<IShaderModule>
    {
        Vector<UInt32> m_ByteCode;
        ShaderModuleDesc m_Desc;
        vk::UniqueShaderModule m_NativeModule;
        VKDevice* m_Device;

    public:
        FE_CLASS_RTTI(VKShaderModule, "823A44B8-72BD-4F19-BCFA-32D077B06B3A");

        VKShaderModule(VKDevice& dev, const ShaderModuleDesc& desc);

        const ShaderModuleDesc& GetDesc() const override;
    };
} // namespace FE
