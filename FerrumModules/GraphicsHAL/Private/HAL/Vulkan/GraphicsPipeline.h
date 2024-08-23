#pragma once
#include <HAL/GraphicsPipeline.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class GraphicsPipeline final : public HAL::GraphicsPipeline
    {
        VkPipelineLayout m_Layout = VK_NULL_HANDLE;
        VkPipeline m_NativePipeline = VK_NULL_HANDLE;

        HAL::GraphicsPipelineDesc m_Desc;

        struct VertexStates
        {
            VkPipelineVertexInputStateCreateInfo VertexInput{};
            VkPipelineInputAssemblyStateCreateInfo InputAssembly{};

            eastl::vector<VkVertexInputBindingDescription> BindingDesc{};
            eastl::vector<VkVertexInputAttributeDescription> AttributeDesc{};
        };

        struct ViewportState
        {
            VkPipelineViewportStateCreateInfo CreateInfo{};
            VkViewport Viewport;
            VkRect2D Scissor;
        };

        struct BlendState
        {
            eastl::vector<VkPipelineColorBlendAttachmentState> Attachments{};
            VkPipelineColorBlendStateCreateInfo CreateInfo{};
        };

        eastl::vector<VkPipelineShaderStageCreateInfo> BuildShaderStages();
        void BuildVertexStates(VertexStates& states) const;
        void BuildViewportState(ViewportState& state) const;
        void BuildBlendState(BlendState& state);
        VkPipelineColorBlendAttachmentState BuildBlendState(uint32_t attachmentIndex);
        VkPipelineRasterizationStateCreateInfo BuildRasterizationState();
        VkPipelineMultisampleStateCreateInfo BuildMultisampleState();
        [[nodiscard]] VkPipelineDepthStencilStateCreateInfo BuildDepthState() const;

    public:
        FE_RTTI_Class(GraphicsPipeline, "4524C98F-C971-47EB-A896-6C4EA33CA549");

        GraphicsPipeline(HAL::Device* pDevice);
        ~GraphicsPipeline() override;

        HAL::ResultCode Init(const HAL::GraphicsPipelineDesc& desc) override;

        [[nodiscard]] inline VkPipeline GetNative() const
        {
            return m_NativePipeline;
        }

        inline VkPipelineLayout GetNativeLayout() const
        {
            return m_Layout;
        }
    };


    FE_ENABLE_IMPL_CAST(GraphicsPipeline);
} // namespace FE::Graphics::Vulkan
