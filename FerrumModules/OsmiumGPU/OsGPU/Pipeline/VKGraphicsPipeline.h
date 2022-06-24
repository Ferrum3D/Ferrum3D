#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Pipeline/IGraphicsPipeline.h>

namespace FE::GPU
{
    class VKDevice;

    class VKGraphicsPipeline : public Object<IGraphicsPipeline>
    {
        VKDevice* m_Device;

        vk::UniquePipelineLayout m_Layout;
        vk::UniquePipeline m_NativePipeline;

        GraphicsPipelineDesc m_Desc;

        struct VertexStates
        {
            vk::PipelineVertexInputStateCreateInfo VertexInput{};
            vk::PipelineInputAssemblyStateCreateInfo InputAssembly{};

            Vector<vk::VertexInputBindingDescription> BindingDesc{};
            Vector<vk::VertexInputAttributeDescription> AttributeDesc{};
        };

        struct ViewportState
        {
            vk::PipelineViewportStateCreateInfo CreateInfo{};
            vk::Viewport Viewport;
            vk::Rect2D Scissor;
        };

        struct BlendState
        {
            Vector<vk::PipelineColorBlendAttachmentState> Attachments{};
            vk::PipelineColorBlendStateCreateInfo CreateInfo{};
        };

        Vector<vk::PipelineShaderStageCreateInfo> BuildShaderStages();
        void BuildVertexStates(VertexStates& states) const;
        void BuildViewportState(ViewportState& state) const;
        void BuildBlendState(BlendState& state);
        vk::PipelineColorBlendAttachmentState BuildBlendState(size_t attachmentIndex);
        vk::PipelineRasterizationStateCreateInfo BuildRasterizationState();
        vk::PipelineMultisampleStateCreateInfo BuildMultisampleState();
        [[nodiscard]] vk::PipelineDepthStencilStateCreateInfo BuildDepthState() const;

    public:
        FE_CLASS_RTTI(VKGraphicsPipeline, "4524C98F-C971-47EB-A896-6C4EA33CA549");

        VKGraphicsPipeline(VKDevice& dev, const GraphicsPipelineDesc& desc);

        inline vk::Pipeline& GetNativePipeline();
        inline vk::PipelineLayout& GetNativeLayout();
    };

    inline vk::Pipeline& VKGraphicsPipeline::GetNativePipeline()
    {
        return m_NativePipeline.get();
    }

    inline vk::PipelineLayout& VKGraphicsPipeline::GetNativeLayout()
    {
        return m_Layout.get();
    }
} // namespace FE::GPU
