#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Shader/IShaderModule.h>

namespace FE::GPU
{
    inline vk::ShaderStageFlags VKConvert(ShaderStageFlags source)
    {
        auto result = static_cast<vk::ShaderStageFlags>(0);
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & ShaderStageFlags::ferrum) != ShaderStageFlags::None)                                                           \
        result |= vk::ShaderStageFlagBits::vulkan

        FE_CVT_ENTRY(Pixel, eFragment);
        FE_CVT_ENTRY(Vertex, eVertex);
        FE_CVT_ENTRY(Hull, eTessellationControl);
        FE_CVT_ENTRY(Domain, eTessellationEvaluation);
        FE_CVT_ENTRY(Geometry, eGeometry);
        FE_CVT_ENTRY(Compute, eCompute);
#undef FE_CVT_ENTRY

        return result;
    }

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
} // namespace FE::GPU
