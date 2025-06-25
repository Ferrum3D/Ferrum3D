#pragma once
#include <Graphics/Core/ComputePipeline.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary;


    struct ComputePipelineInitContext final
    {
        Env::Name m_defines;
        festd::inline_vector<Core::ShaderSpecializationConstant> m_specializationConstants;
        Core::ComputePipelineDesc m_desc;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_bindlessSetLayout = VK_NULL_HANDLE;
        ShaderLibrary* m_shaderLibrary = nullptr;
        Logger* m_logger = nullptr;
    };


    struct ComputePipeline final : public Core::ComputePipeline
    {
        FE_RTTI_Class(ComputePipeline, "0ED571F4-58C9-40D9-AA58-C70450FB0E6A");

        using InitContext = ComputePipelineInitContext;

        ComputePipeline(Core::Device* device);
        ~ComputePipeline() override;

        void InitInternal(const ComputePipelineInitContext& context);

        [[nodiscard]] VkPipeline GetNative() const
        {
            return m_nativePipeline;
        }

        [[nodiscard]] VkPipelineLayout GetNativeLayout() const
        {
            return m_layout;
        }

    private:
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
        VkPipeline m_nativePipeline = VK_NULL_HANDLE;
    };

    FE_ENABLE_IMPL_CAST(ComputePipeline);
} // namespace FE::Graphics::Vulkan
