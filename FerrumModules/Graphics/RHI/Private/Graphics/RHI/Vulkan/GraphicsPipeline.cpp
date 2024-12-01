#include <FeCore/Containers/SmallVector.h>
#include <Graphics/RHI/Vulkan/CommandList.h>
#include <Graphics/RHI/Vulkan/Common/Viewport.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/GraphicsPipeline.h>
#include <Graphics/RHI/Vulkan/ImageFormat.h>
#include <Graphics/RHI/Vulkan/PipelineStates.h>
#include <Graphics/RHI/Vulkan/RenderPass.h>
#include <Graphics/RHI/Vulkan/ShaderModule.h>
#include <Graphics/RHI/Vulkan/ShaderReflection.h>
#include <Graphics/RHI/Vulkan/ShaderResourceGroup.h>

namespace FE::Graphics::Vulkan
{
    GraphicsPipeline::GraphicsPipeline(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode GraphicsPipeline::Init(const RHI::GraphicsPipelineDesc& desc)
    {
        m_desc = desc;

        festd::small_vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(desc.m_shaderResourceGroups.size());
        for (const RHI::ShaderResourceGroup* srg : desc.m_shaderResourceGroups)
        {
            setLayouts.push_back(ImplCast(srg)->GetNativeSetLayout());
        }

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.pSetLayouts = setLayouts.data();
        layoutCI.setLayoutCount = setLayouts.size();
        vkCreatePipelineLayout(NativeCast(m_device), &layoutCI, VK_NULL_HANDLE, &m_layout);

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.layout = m_layout;

        auto shaderStages = BuildShaderStages();
        pipelineCI.stageCount = shaderStages.size();
        pipelineCI.pStages = shaderStages.data();

        VertexStates vertexStates{};
        BuildVertexStates(vertexStates);
        pipelineCI.pVertexInputState = &vertexStates.m_vertexInput;
        pipelineCI.pInputAssemblyState = &vertexStates.m_inputAssembly;

        ViewportState viewport{};
        BuildViewportState(viewport);
        pipelineCI.pViewportState = &viewport.m_createInfo;

        BlendState blend{};
        BuildBlendState(blend);
        pipelineCI.pColorBlendState = &blend.m_createInfo;

        auto rasterization = BuildRasterizationState();
        pipelineCI.pRasterizationState = &rasterization;
        auto ms = BuildMultisampleState();
        pipelineCI.pMultisampleState = &ms;
        auto depth = BuildDepthState();
        pipelineCI.pDepthStencilState = &depth;

        const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.dynamicStateCount = 2;
        dynamicStateCI.pDynamicStates = dynamicState;
        pipelineCI.pDynamicState = &dynamicStateCI;

        pipelineCI.layout = m_layout;
        pipelineCI.renderPass = NativeCast(m_desc.m_renderPass);
        pipelineCI.subpass = m_desc.m_subpassIndex;

        vkCreateGraphicsPipelines(NativeCast(m_device), VK_NULL_HANDLE, 1, &pipelineCI, VK_NULL_HANDLE, &m_nativePipeline);
        return RHI::ResultCode::kSuccess;
    }


    void GraphicsPipeline::BuildVertexStates(VertexStates& states) const
    {
        const auto& buffers = m_desc.m_inputLayout.m_buffers;
        const auto& attributes = m_desc.m_inputLayout.m_attributes;

        for (uint32_t i = 0; i < buffers.size(); ++i)
        {
            auto& binding = states.m_bindingDesc.push_back();
            binding.binding = i;
            binding.inputRate = VKConvert(buffers[i].m_inputRate);
            binding.stride = buffers[i].m_stride;
        }

        RHI::ShaderModule* vertexShader = nullptr;
        for (RHI::ShaderModule* shader : m_desc.m_shaders)
        {
            if (shader->GetDesc().m_stage == RHI::ShaderStage::kVertex)
                vertexShader = shader;
        }

        for (const auto& item : attributes)
        {
            auto& attribute = states.m_attributeDesc.push_back();
            attribute.binding = item.m_bufferIndex;
            attribute.location = vertexShader->GetReflection()->GetInputAttributeLocation(item.m_shaderSemantic);
            attribute.offset = item.m_offset;
            attribute.format = VKConvert(item.m_elementFormat);
        }

        states.m_vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        states.m_vertexInput.pVertexBindingDescriptions = states.m_bindingDesc.data();
        states.m_vertexInput.vertexBindingDescriptionCount = states.m_bindingDesc.size();
        states.m_vertexInput.pVertexAttributeDescriptions = states.m_attributeDesc.data();
        states.m_vertexInput.vertexAttributeDescriptionCount = states.m_attributeDesc.size();

        states.m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        states.m_inputAssembly.topology = VKConvert(m_desc.m_inputLayout.m_topology);
        states.m_inputAssembly.primitiveRestartEnable = false;
    }


    eastl::vector<VkPipelineShaderStageCreateInfo> GraphicsPipeline::BuildShaderStages()
    {
        eastl::vector<VkPipelineShaderStageCreateInfo> result;
        for (RHI::ShaderModule* shader : m_desc.m_shaders)
            result.push_back(ImplCast(shader)->GetStageCI());

        return result;
    }


    void GraphicsPipeline::BuildViewportState(ViewportState& state) const
    {
        state.m_viewport = VKConvert(m_desc.m_viewport);
        state.m_scissor = VKConvert(m_desc.m_scissor);

        state.m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        state.m_createInfo.pViewports = &state.m_viewport;
        state.m_createInfo.viewportCount = 1;
        state.m_createInfo.pScissors = &state.m_scissor;
        state.m_createInfo.scissorCount = 1;
    }


    VkPipelineRasterizationStateCreateInfo GraphicsPipeline::BuildRasterizationState()
    {
        const RHI::RasterizationState& rasterization = m_desc.m_rasterization;

        VkPipelineRasterizationStateCreateInfo result{};
        result.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        result.depthClampEnable = rasterization.m_depthClampEnabled;
        result.depthBiasEnable = rasterization.m_depthBiasEnabled;
        result.rasterizerDiscardEnable = rasterization.m_rasterDiscardEnabled;
        result.cullMode = VKConvert(rasterization.m_cullMode);
        result.polygonMode = VKConvert(rasterization.m_polyMode);
        result.lineWidth = 1.0f;
        result.frontFace = VK_FRONT_FACE_CLOCKWISE;
        return result;
    }


    VkPipelineMultisampleStateCreateInfo GraphicsPipeline::BuildMultisampleState()
    {
        const RHI::MultisampleState& multisample = m_desc.m_multisample;

        VkPipelineMultisampleStateCreateInfo result{};
        result.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        result.sampleShadingEnable = multisample.m_sampleShadingEnabled;
        result.rasterizationSamples = GetVKSampleCountFlags(multisample.m_sampleCount);
        result.minSampleShading = multisample.m_minSampleShading;
        result.pSampleMask = nullptr;
        result.alphaToCoverageEnable = false;
        result.alphaToOneEnable = false;
        return result;
    }


    VkPipelineDepthStencilStateCreateInfo GraphicsPipeline::BuildDepthState() const
    {
        VkPipelineDepthStencilStateCreateInfo result{};
        result.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        result.depthTestEnable = m_desc.m_depthStencil.m_depthTestEnabled;
        result.depthWriteEnable = m_desc.m_depthStencil.m_depthWriteEnabled;
        result.depthCompareOp = VKConvert(m_desc.m_depthStencil.m_depthCompareOp);
        result.depthBoundsTestEnable = false;
        result.minDepthBounds = 0.0f;
        result.maxDepthBounds = 1.0f;
        return result;
    }


    VkPipelineColorBlendAttachmentState GraphicsPipeline::BuildBlendState(uint32_t attachmentIndex)
    {
        auto& state = m_desc.m_colorBlend.m_targetBlendStates[attachmentIndex];

        VkPipelineColorBlendAttachmentState result{};
        result.blendEnable = state.m_blendEnabled;
        result.colorWriteMask = VKConvert(state.m_colorWriteFlags);
        result.srcColorBlendFactor = VKConvert(state.m_sourceFactor);
        result.srcAlphaBlendFactor = VKConvert(state.m_sourceAlphaFactor);
        result.dstColorBlendFactor = VKConvert(state.m_destinationFactor);
        result.dstAlphaBlendFactor = VKConvert(state.m_destinationAlphaFactor);
        result.colorBlendOp = VKConvert(state.m_blendOp);
        result.alphaBlendOp = VKConvert(state.m_alphaBlendOp);
        return result;
    }


    void GraphicsPipeline::BuildBlendState(BlendState& state)
    {
        for (uint32_t i = 0; i < m_desc.m_colorBlend.m_targetBlendStates.size(); ++i)
            state.m_attachments.push_back(BuildBlendState(i));

        state.m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        state.m_createInfo.logicOpEnable = false;
        state.m_createInfo.logicOp = VK_LOGIC_OP_COPY;
        state.m_createInfo.pAttachments = state.m_attachments.data();
        state.m_createInfo.attachmentCount = state.m_attachments.size();

        memcpy(state.m_createInfo.blendConstants, m_desc.m_colorBlend.m_blendConstants.Data(), sizeof(Vector4F));
    }


    GraphicsPipeline::~GraphicsPipeline()
    {
        const VkDevice device = NativeCast(m_device);
        vkDestroyPipeline(device, m_nativePipeline, nullptr);
        vkDestroyPipelineLayout(device, m_layout, nullptr);
    }
} // namespace FE::Graphics::Vulkan
