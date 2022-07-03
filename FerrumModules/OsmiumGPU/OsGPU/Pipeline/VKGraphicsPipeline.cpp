#include <OsGPU/CommandBuffer/VKCommandBuffer.h>
#include <OsGPU/Common/VKViewport.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/Pipeline/VKGraphicsPipeline.h>
#include <OsGPU/Pipeline/VKPipelineStates.h>
#include <OsGPU/RenderPass/VKRenderPass.h>
#include <OsGPU/Shader/VKShaderModule.h>
#include <OsGPU/Shader/VKShaderReflection.h>

namespace FE::Osmium
{
    VKGraphicsPipeline::VKGraphicsPipeline(VKDevice& dev, const GraphicsPipelineDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        Vector<vk::DescriptorSetLayout> setLayouts;
        for (auto& table : desc.DescriptorTables)
        {
            auto* vkTable = fe_assert_cast<VKDescriptorTable*>(table.GetRaw());
            setLayouts.push_back(vkTable->GetNativeSetLayout());
        }

        vk::PipelineLayoutCreateInfo layoutCI{};
        layoutCI.pSetLayouts    = setLayouts.data();
        layoutCI.setLayoutCount = static_cast<UInt32>(setLayouts.size());
        m_Layout                = m_Device->GetNativeDevice().createPipelineLayoutUnique(layoutCI);

        vk::GraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.layout = m_Layout.get();

        auto shaderStages     = BuildShaderStages();
        pipelineCI.stageCount = static_cast<UInt32>(shaderStages.size());
        pipelineCI.pStages    = shaderStages.data();

        VertexStates vertexStates{};
        BuildVertexStates(vertexStates);
        pipelineCI.pVertexInputState   = &vertexStates.VertexInput;
        pipelineCI.pInputAssemblyState = &vertexStates.InputAssembly;

        ViewportState viewport{};
        BuildViewportState(viewport);
        pipelineCI.pViewportState = &viewport.CreateInfo;

        BlendState blend{};
        BuildBlendState(blend);
        pipelineCI.pColorBlendState = &blend.CreateInfo;

        auto rasterization             = BuildRasterizationState();
        pipelineCI.pRasterizationState = &rasterization;
        auto ms                        = BuildMultisampleState();
        pipelineCI.pMultisampleState   = &ms;
        auto depth                     = BuildDepthState();
        pipelineCI.pDepthStencilState  = &depth;

        vk::DynamicState dynamicState[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

        vk::PipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.dynamicStateCount = 2;
        dynamicStateCI.pDynamicStates    = dynamicState;
        pipelineCI.pDynamicState         = &dynamicStateCI;

        pipelineCI.layout     = m_Layout.get();
        pipelineCI.renderPass = fe_assert_cast<VKRenderPass*>(m_Desc.RenderPass.GetRaw())->GetNativeRenderPass();
        pipelineCI.subpass    = m_Desc.SubpassIndex;

        m_NativePipeline = m_Device->GetNativeDevice().createGraphicsPipelineUnique(nullptr, pipelineCI).value;
    }

    void VKGraphicsPipeline::BuildVertexStates(VKGraphicsPipeline::VertexStates& states) const
    {
        const auto& buffers    = m_Desc.InputLayout.GetBuffers();
        const auto& attributes = m_Desc.InputLayout.GetAttributes();

        for (size_t i = 0; i < buffers.Size(); ++i)
        {
            auto& binding     = states.BindingDesc.emplace_back();
            binding.binding   = static_cast<UInt32>(i);
            binding.inputRate = VKConvert(buffers[i].InputRate);
            binding.stride    = buffers[i].Stride;
        }

        IShaderModule* vertexShader = nullptr;
        for (auto& shader : m_Desc.Shaders)
        {
            if (shader->GetDesc().Stage == ShaderStage::Vertex)
            {
                vertexShader = shader.GetRaw();
            }
        }

        for (const auto& item : attributes)
        {
            auto& attribute    = states.AttributeDesc.emplace_back();
            attribute.binding  = item.BufferIndex;
            attribute.location = vertexShader->GetReflection()->GetInputAttributeLocation(item.ShaderSemantic);
            attribute.offset   = item.Offset;
            attribute.format   = VKConvert(item.ElementFormat);
        }

        states.VertexInput.pVertexBindingDescriptions      = states.BindingDesc.data();
        states.VertexInput.vertexBindingDescriptionCount   = static_cast<UInt32>(states.BindingDesc.size());
        states.VertexInput.pVertexAttributeDescriptions    = states.AttributeDesc.data();
        states.VertexInput.vertexAttributeDescriptionCount = static_cast<UInt32>(states.AttributeDesc.size());

        states.InputAssembly.topology               = VKConvert(m_Desc.InputLayout.Topology);
        states.InputAssembly.primitiveRestartEnable = false;
    }

    Vector<vk::PipelineShaderStageCreateInfo> VKGraphicsPipeline::BuildShaderStages()
    {
        Vector<vk::PipelineShaderStageCreateInfo> result;
        for (auto& shader : m_Desc.Shaders)
        {
            auto* vkShader = fe_assert_cast<VKShaderModule*>(shader.GetRaw());
            result.push_back(vkShader->GetStageCI());
        }

        return result;
    }

    void VKGraphicsPipeline::BuildViewportState(VKGraphicsPipeline::ViewportState& state) const
    {
        state.Viewport = VKConvert(m_Desc.Viewport);
        state.Scissor  = VKConvert(m_Desc.Scissor);

        state.CreateInfo.pViewports    = &state.Viewport;
        state.CreateInfo.viewportCount = 1;
        state.CreateInfo.pScissors     = &state.Scissor;
        state.CreateInfo.scissorCount  = 1;
    }

    vk::PipelineRasterizationStateCreateInfo VKGraphicsPipeline::BuildRasterizationState()
    {
        RasterizationState& rasterization = m_Desc.Rasterization;

        vk::PipelineRasterizationStateCreateInfo result{};
        result.depthClampEnable        = rasterization.DepthClampEnabled;
        result.depthBiasEnable         = rasterization.DepthBiasEnabled;
        result.rasterizerDiscardEnable = rasterization.RasterDiscardEnabled;
        result.cullMode                = VKConvert(rasterization.CullMode);
        result.polygonMode             = VKConvert(rasterization.PolyMode);
        result.lineWidth               = 1.0f;
        result.frontFace               = vk::FrontFace::eClockwise;
        return result;
    }

    vk::PipelineMultisampleStateCreateInfo VKGraphicsPipeline::BuildMultisampleState()
    {
        MultisampleState& multisample = m_Desc.Multisample;

        vk::PipelineMultisampleStateCreateInfo result{};
        result.sampleShadingEnable   = multisample.SampleShadingEnabled;
        result.rasterizationSamples  = GetVKSampleCountFlags(multisample.SampleCount);
        result.minSampleShading      = multisample.MinSampleShading;
        result.pSampleMask           = nullptr;
        result.alphaToCoverageEnable = false;
        result.alphaToOneEnable      = false;
        return result;
    }

    vk::PipelineDepthStencilStateCreateInfo VKGraphicsPipeline::BuildDepthState() const
    {
        vk::PipelineDepthStencilStateCreateInfo result{};
        result.depthTestEnable       = m_Desc.DepthStencil.DepthTestEnabled;
        result.depthWriteEnable      = m_Desc.DepthStencil.DepthWriteEnabled;
        result.depthCompareOp        = VKConvert(m_Desc.DepthStencil.DepthCompareOp);
        result.depthBoundsTestEnable = false;
        result.minDepthBounds        = 0.0f;
        result.maxDepthBounds        = 1.0f;
        return result;
    }

    vk::PipelineColorBlendAttachmentState VKGraphicsPipeline::BuildBlendState(size_t attachmentIndex)
    {
        auto& state = m_Desc.ColorBlend.TargetBlendStates[attachmentIndex];

        vk::PipelineColorBlendAttachmentState result{};
        result.blendEnable         = state.BlendEnabled;
        result.colorWriteMask      = VKConvert(state.ColorWriteFlags);
        result.srcColorBlendFactor = VKConvert(state.SourceFactor);
        result.srcAlphaBlendFactor = VKConvert(state.SourceAlphaFactor);
        result.dstColorBlendFactor = VKConvert(state.DestinationFactor);
        result.dstAlphaBlendFactor = VKConvert(state.DestinationAlphaFactor);
        result.colorBlendOp        = VKConvert(state.BlendOp);
        result.alphaBlendOp        = VKConvert(state.AlphaBlendOp);
        return result;
    }

    void VKGraphicsPipeline::BuildBlendState(VKGraphicsPipeline::BlendState& state)
    {
        for (size_t i = 0; i < m_Desc.ColorBlend.TargetBlendStates.Size(); ++i)
        {
            state.Attachments.push_back(BuildBlendState(i));
        }

        state.CreateInfo.logicOpEnable   = false;
        state.CreateInfo.logicOp         = vk::LogicOp::eCopy;
        state.CreateInfo.pAttachments    = state.Attachments.data();
        state.CreateInfo.attachmentCount = static_cast<UInt32>(state.Attachments.size());

        state.CreateInfo.blendConstants[0] = m_Desc.ColorBlend.BlendConstants.X();
        state.CreateInfo.blendConstants[1] = m_Desc.ColorBlend.BlendConstants.Y();
        state.CreateInfo.blendConstants[2] = m_Desc.ColorBlend.BlendConstants.Z();
        state.CreateInfo.blendConstants[3] = m_Desc.ColorBlend.BlendConstants.W();
    }
} // namespace FE::Osmium
