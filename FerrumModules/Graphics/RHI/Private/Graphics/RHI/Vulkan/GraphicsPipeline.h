#pragma once
#include <Graphics/RHI/GraphicsPipeline.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct GraphicsPipeline final : public RHI::GraphicsPipeline
    {
        FE_RTTI_Class(GraphicsPipeline, "4524C98F-C971-47EB-A896-6C4EA33CA549");

        GraphicsPipeline(RHI::Device* device);
        ~GraphicsPipeline() override;

        RHI::ResultCode Init(const RHI::GraphicsPipelineDesc& desc) override;

        [[nodiscard]] VkPipeline GetNative() const
        {
            return m_nativePipeline;
        }

        [[nodiscard]] VkPipelineLayout GetNativeLayout() const
        {
            return m_layout;
        }

    private:
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
        VkPipeline m_nativePipeline = VK_NULL_HANDLE;

        RHI::GraphicsPipelineDesc m_desc;

        struct VertexStates final
        {
            VkPipelineVertexInputStateCreateInfo m_vertexInput{};
            VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};

            festd::vector<VkVertexInputBindingDescription> m_bindingDesc{};
            festd::vector<VkVertexInputAttributeDescription> m_attributeDesc{};
        };

        struct ViewportState final
        {
            VkPipelineViewportStateCreateInfo m_createInfo{};
            VkViewport m_viewport;
            VkRect2D m_scissor;
        };

        struct BlendState final
        {
            festd::vector<VkPipelineColorBlendAttachmentState> m_attachments{};
            VkPipelineColorBlendStateCreateInfo m_createInfo{};
        };

        festd::vector<VkPipelineShaderStageCreateInfo> BuildShaderStages();
        void BuildVertexStates(VertexStates& states) const;
        void BuildViewportState(ViewportState& state) const;
        void BuildBlendState(BlendState& state);
        VkPipelineColorBlendAttachmentState BuildBlendState(uint32_t attachmentIndex);
        VkPipelineRasterizationStateCreateInfo BuildRasterizationState();
        VkPipelineMultisampleStateCreateInfo BuildMultisampleState();
        [[nodiscard]] VkPipelineDepthStencilStateCreateInfo BuildDepthState() const;
    };


    FE_ENABLE_NATIVE_CAST(GraphicsPipeline);
} // namespace FE::Graphics::Vulkan
