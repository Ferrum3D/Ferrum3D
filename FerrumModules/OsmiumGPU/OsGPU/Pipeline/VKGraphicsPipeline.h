#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Pipeline/IGraphicsPipeline.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKGraphicsPipeline : public IGraphicsPipeline
    {
        VKDevice* m_Device;

        VkPipelineLayout m_Layout;
        VkPipeline m_NativePipeline;

        GraphicsPipelineDesc m_Desc;

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
        FE_CLASS_RTTI(VKGraphicsPipeline, "4524C98F-C971-47EB-A896-6C4EA33CA549");

        VKGraphicsPipeline(VKDevice& dev, const GraphicsPipelineDesc& desc);
        ~VKGraphicsPipeline() override;

        inline VkPipeline GetNativePipeline();
        inline VkPipelineLayout GetNativeLayout();
    };

    inline VkPipeline VKGraphicsPipeline::GetNativePipeline()
    {
        return m_NativePipeline;
    }

    inline VkPipelineLayout VKGraphicsPipeline::GetNativeLayout()
    {
        return m_Layout;
    }
} // namespace FE::Osmium
