#pragma once
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary;


    struct GraphicsPipelineInitContext final
    {
        Env::Name m_defines;
        festd::inline_vector<Core::ShaderSpecializationConstant> m_specializationConstants;
        Core::GraphicsPipelineDesc m_desc;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_bindlessSetLayout = VK_NULL_HANDLE;
        ShaderLibrary* m_shaderLibrary = nullptr;
        Logger* m_logger = nullptr;
    };


    struct GraphicsPipeline final : public Core::GraphicsPipeline
    {
        FE_RTTI_Class(GraphicsPipeline, "4524C98F-C971-47EB-A896-6C4EA33CA549");

        using InitContext = GraphicsPipelineInitContext;

        GraphicsPipeline(Core::Device* device);
        ~GraphicsPipeline() override;

        void InitInternal(const GraphicsPipelineInitContext& context);

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


    FE_ENABLE_NATIVE_CAST(GraphicsPipeline);
} // namespace FE::Graphics::Vulkan
