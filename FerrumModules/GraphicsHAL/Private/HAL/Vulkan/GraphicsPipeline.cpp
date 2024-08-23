#include <FeCore/Containers/SmallVector.h>
#include <HAL/Vulkan/CommandList.h>
#include <HAL/Vulkan/Common/Viewport.h>
#include <HAL/Vulkan/DescriptorTable.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/GraphicsPipeline.h>
#include <HAL/Vulkan/ImageFormat.h>
#include <HAL/Vulkan/PipelineStates.h>
#include <HAL/Vulkan/RenderPass.h>
#include <HAL/Vulkan/ShaderModule.h>
#include <HAL/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    GraphicsPipeline::GraphicsPipeline(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode GraphicsPipeline::Init(const HAL::GraphicsPipelineDesc& desc)
    {
        m_Desc = desc;

        festd::small_vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(desc.DescriptorTables.size());
        for (const HAL::DescriptorTable* table : desc.DescriptorTables)
        {
            setLayouts.push_back(ImplCast(table)->GetNativeSetLayout());
        }

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.pSetLayouts = setLayouts.data();
        layoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        vkCreatePipelineLayout(NativeCast(m_pDevice), &layoutCI, VK_NULL_HANDLE, &m_Layout);

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.layout = m_Layout;

        auto shaderStages = BuildShaderStages();
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        VertexStates vertexStates{};
        BuildVertexStates(vertexStates);
        pipelineCI.pVertexInputState = &vertexStates.VertexInput;
        pipelineCI.pInputAssemblyState = &vertexStates.InputAssembly;

        ViewportState viewport{};
        BuildViewportState(viewport);
        pipelineCI.pViewportState = &viewport.CreateInfo;

        BlendState blend{};
        BuildBlendState(blend);
        pipelineCI.pColorBlendState = &blend.CreateInfo;

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

        pipelineCI.layout = m_Layout;
        pipelineCI.renderPass = NativeCast(m_Desc.RenderPass);
        pipelineCI.subpass = m_Desc.SubpassIndex;

        vkCreateGraphicsPipelines(NativeCast(m_pDevice), VK_NULL_HANDLE, 1, &pipelineCI, VK_NULL_HANDLE, &m_NativePipeline);
        return HAL::ResultCode::Success;
    }


    void GraphicsPipeline::BuildVertexStates(GraphicsPipeline::VertexStates& states) const
    {
        const auto& buffers = m_Desc.InputLayout.GetBuffers();
        const auto& attributes = m_Desc.InputLayout.GetAttributes();

        for (uint32_t i = 0; i < buffers.size(); ++i)
        {
            auto& binding = states.BindingDesc.push_back();
            binding.binding = static_cast<uint32_t>(i);
            binding.inputRate = VKConvert(buffers[i].InputRate);
            binding.stride = buffers[i].Stride;
        }

        HAL::ShaderModule* vertexShader = nullptr;
        for (auto& shader : m_Desc.Shaders)
        {
            if (shader->GetDesc().Stage == HAL::ShaderStage::Vertex)
            {
                vertexShader = shader;
            }
        }

        for (const auto& item : attributes)
        {
            auto& attribute = states.AttributeDesc.push_back();
            attribute.binding = item.BufferIndex;
            attribute.location = vertexShader->GetReflection()->GetInputAttributeLocation(item.ShaderSemantic);
            attribute.offset = item.Offset;
            attribute.format = VKConvert(item.ElementFormat);
        }

        states.VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        states.VertexInput.pVertexBindingDescriptions = states.BindingDesc.data();
        states.VertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(states.BindingDesc.size());
        states.VertexInput.pVertexAttributeDescriptions = states.AttributeDesc.data();
        states.VertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(states.AttributeDesc.size());

        states.InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        states.InputAssembly.topology = VKConvert(m_Desc.InputLayout.Topology);
        states.InputAssembly.primitiveRestartEnable = false;
    }


    eastl::vector<VkPipelineShaderStageCreateInfo> GraphicsPipeline::BuildShaderStages()
    {
        eastl::vector<VkPipelineShaderStageCreateInfo> result;
        for (HAL::ShaderModule* shader : m_Desc.Shaders)
        {
            result.push_back(ImplCast(shader)->GetStageCI());
        }

        return result;
    }


    void GraphicsPipeline::BuildViewportState(GraphicsPipeline::ViewportState& state) const
    {
        state.Viewport = VKConvert(m_Desc.Viewport);
        state.Scissor = VKConvert(m_Desc.Scissor);

        state.CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        state.CreateInfo.pViewports = &state.Viewport;
        state.CreateInfo.viewportCount = 1;
        state.CreateInfo.pScissors = &state.Scissor;
        state.CreateInfo.scissorCount = 1;
    }


    VkPipelineRasterizationStateCreateInfo GraphicsPipeline::BuildRasterizationState()
    {
        HAL::RasterizationState& rasterization = m_Desc.Rasterization;

        VkPipelineRasterizationStateCreateInfo result{};
        result.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        result.depthClampEnable = rasterization.DepthClampEnabled;
        result.depthBiasEnable = rasterization.DepthBiasEnabled;
        result.rasterizerDiscardEnable = rasterization.RasterDiscardEnabled;
        result.cullMode = VKConvert(rasterization.CullMode);
        result.polygonMode = VKConvert(rasterization.PolyMode);
        result.lineWidth = 1.0f;
        result.frontFace = VK_FRONT_FACE_CLOCKWISE;
        return result;
    }


    VkPipelineMultisampleStateCreateInfo GraphicsPipeline::BuildMultisampleState()
    {
        HAL::MultisampleState& multisample = m_Desc.Multisample;

        VkPipelineMultisampleStateCreateInfo result{};
        result.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        result.sampleShadingEnable = multisample.SampleShadingEnabled;
        result.rasterizationSamples = GetVKSampleCountFlags(multisample.SampleCount);
        result.minSampleShading = multisample.MinSampleShading;
        result.pSampleMask = nullptr;
        result.alphaToCoverageEnable = false;
        result.alphaToOneEnable = false;
        return result;
    }


    VkPipelineDepthStencilStateCreateInfo GraphicsPipeline::BuildDepthState() const
    {
        VkPipelineDepthStencilStateCreateInfo result{};
        result.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        result.depthTestEnable = m_Desc.DepthStencil.DepthTestEnabled;
        result.depthWriteEnable = m_Desc.DepthStencil.DepthWriteEnabled;
        result.depthCompareOp = VKConvert(m_Desc.DepthStencil.DepthCompareOp);
        result.depthBoundsTestEnable = false;
        result.minDepthBounds = 0.0f;
        result.maxDepthBounds = 1.0f;
        return result;
    }


    VkPipelineColorBlendAttachmentState GraphicsPipeline::BuildBlendState(uint32_t attachmentIndex)
    {
        auto& state = m_Desc.ColorBlend.TargetBlendStates[attachmentIndex];

        VkPipelineColorBlendAttachmentState result{};
        result.blendEnable = state.BlendEnabled;
        result.colorWriteMask = VKConvert(state.ColorWriteFlags);
        result.srcColorBlendFactor = VKConvert(state.SourceFactor);
        result.srcAlphaBlendFactor = VKConvert(state.SourceAlphaFactor);
        result.dstColorBlendFactor = VKConvert(state.DestinationFactor);
        result.dstAlphaBlendFactor = VKConvert(state.DestinationAlphaFactor);
        result.colorBlendOp = VKConvert(state.BlendOp);
        result.alphaBlendOp = VKConvert(state.AlphaBlendOp);
        return result;
    }


    void GraphicsPipeline::BuildBlendState(GraphicsPipeline::BlendState& state)
    {
        for (uint32_t i = 0; i < m_Desc.ColorBlend.TargetBlendStates.size(); ++i)
        {
            state.Attachments.push_back(BuildBlendState(i));
        }

        state.CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        state.CreateInfo.logicOpEnable = false;
        state.CreateInfo.logicOp = VK_LOGIC_OP_COPY;
        state.CreateInfo.pAttachments = state.Attachments.data();
        state.CreateInfo.attachmentCount = static_cast<uint32_t>(state.Attachments.size());

        state.CreateInfo.blendConstants[0] = m_Desc.ColorBlend.BlendConstants.X();
        state.CreateInfo.blendConstants[1] = m_Desc.ColorBlend.BlendConstants.Y();
        state.CreateInfo.blendConstants[2] = m_Desc.ColorBlend.BlendConstants.Z();
        state.CreateInfo.blendConstants[3] = m_Desc.ColorBlend.BlendConstants.W();
    }


    GraphicsPipeline::~GraphicsPipeline()
    {
        const VkDevice device = NativeCast(m_pDevice);
        vkDestroyPipeline(device, m_NativePipeline, nullptr);
        vkDestroyPipelineLayout(device, m_Layout, nullptr);
    }
} // namespace FE::Graphics::Vulkan
