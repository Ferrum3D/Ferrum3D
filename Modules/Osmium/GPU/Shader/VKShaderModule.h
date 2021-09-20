#pragma once
#include <FeCore/Memory/Memory.h>
#include <GPU/Common/VKConfig.h>
#include <GPU/Shader/IShaderModule.h>

namespace FE::GPU
{
    class VKShaderReflection;

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

    inline vk::ShaderStageFlagBits VKConvert(ShaderStage source)
    {
        switch (source)
        {
        case ShaderStage::Vertex:
            return vk::ShaderStageFlagBits::eVertex;
        case ShaderStage::Pixel:
            return vk::ShaderStageFlagBits::eFragment;
        case ShaderStage::Hull:
            return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderStage::Domain:
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case ShaderStage::Geometry:
            return vk::ShaderStageFlagBits::eGeometry;
        case ShaderStage::Compute:
            return vk::ShaderStageFlagBits::eCompute;
        default:
            FE_UNREACHABLE("Invalid ShaderStage");
            return static_cast<vk::ShaderStageFlagBits>(-1);
        }
    }

    class VKDevice;

    class VKShaderModule : public Object<IShaderModule>
    {
        Vector<UInt32> m_ByteCode;
        ShaderModuleDesc m_Desc;
        vk::UniqueShaderModule m_NativeModule;
        VKDevice* m_Device;
        Shared<VKShaderReflection> m_Reflection;

    public:
        FE_CLASS_RTTI(VKShaderModule, "823A44B8-72BD-4F19-BCFA-32D077B06B3A");

        VKShaderModule(VKDevice& dev, const ShaderModuleDesc& desc);

        const ShaderModuleDesc& GetDesc() const override;

        vk::PipelineShaderStageCreateInfo GetStageCI();
        IShaderReflection* GetReflection() override;
    };
} // namespace FE::GPU
