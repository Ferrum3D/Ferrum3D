#include <GPU/CommandBuffer/VKCommandBuffer.h>
#include <GPU/Descriptors/VKDescriptorTable.h>
#include <GPU/Device/VKDevice.h>
#include <GPU/Image/VKImageFormat.h>
#include <GPU/Pipeline/VKGraphicsPipeline.h>
#include <GPU/RenderPass/VKRenderPass.h>
#include <GPU/Shader/VKShaderModule.h>
#include <GPU/Shader/VKShaderReflection.h>
#include <GPU/Common/VKViewport.h>

namespace FE::GPU
{
    inline vk::BlendOp VKConvert(BlendOperation source)
    {
        switch (source)
        {
        case BlendOperation::Add:
            return vk::BlendOp::eAdd;
        case BlendOperation::Subtract:
            return vk::BlendOp::eSubtract;
        case BlendOperation::ReverseSubtract:
            return vk::BlendOp::eReverseSubtract;
        case BlendOperation::Min:
            return vk::BlendOp::eMin;
        case BlendOperation::Max:
            return vk::BlendOp::eMax;
        default:
            FE_UNREACHABLE("Invalid BlendOp");
            return static_cast<vk::BlendOp>(-1);
        }
    }

    inline vk::BlendFactor VKConvert(BlendFactor source)
    {
        switch (source)
        {
        case BlendFactor::Zero:
            return vk::BlendFactor::eZero;
        case BlendFactor::One:
            return vk::BlendFactor::eOne;
        case BlendFactor::SrcColor:
            return vk::BlendFactor::eSrcColor;
        case BlendFactor::OneMinusSrcColor:
            return vk::BlendFactor::eOneMinusSrcColor;
        case BlendFactor::DstColor:
            return vk::BlendFactor::eDstColor;
        case BlendFactor::OneMinusDstColor:
            return vk::BlendFactor::eOneMinusDstColor;
        case BlendFactor::SrcAlpha:
            return vk::BlendFactor::eSrcAlpha;
        case BlendFactor::OneMinusSrcAlpha:
            return vk::BlendFactor::eOneMinusSrcAlpha;
        case BlendFactor::DstAlpha:
            return vk::BlendFactor::eDstAlpha;
        case BlendFactor::OneMinusDstAlpha:
            return vk::BlendFactor::eOneMinusDstAlpha;
        case BlendFactor::ConstantColor:
            return vk::BlendFactor::eConstantColor;
        case BlendFactor::OneMinusConstantColor:
            return vk::BlendFactor::eOneMinusConstantColor;
        case BlendFactor::ConstantAlpha:
            return vk::BlendFactor::eConstantAlpha;
        case BlendFactor::OneMinusConstantAlpha:
            return vk::BlendFactor::eOneMinusConstantAlpha;
        case BlendFactor::SrcAlphaSaturate:
            return vk::BlendFactor::eSrcAlphaSaturate;
        case BlendFactor::Src1Color:
            return vk::BlendFactor::eSrc1Color;
        case BlendFactor::OneMinusSrc1Color:
            return vk::BlendFactor::eOneMinusSrc1Color;
        case BlendFactor::Src1Alpha:
            return vk::BlendFactor::eSrc1Alpha;
        case BlendFactor::OneMinusSrc1Alpha:
            return vk::BlendFactor::eOneMinusSrc1Alpha;
        default:
            FE_UNREACHABLE("Invalid BlendFactor");
            return static_cast<vk::BlendFactor>(-1);
        }
    }

    inline vk::ColorComponentFlags VKConvert(ColorComponentFlags source)
    {
        auto result = static_cast<vk::ColorComponentFlags>(0);
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & ColorComponentFlags::ferrum) != ColorComponentFlags::None)                                                     \
    result |= vk::ColorComponentFlagBits::vulkan
        FE_CVT_ENTRY(Red, eR);
        FE_CVT_ENTRY(Green, eG);
        FE_CVT_ENTRY(Blue, eB);
        FE_CVT_ENTRY(Alpha, eA);
#undef FE_CVT_ENTRY
        return result;
    }

    inline vk::CompareOp VKConvert(CompareOp source)
    {
        switch (source)
        {
        case CompareOp::Never:
            return vk::CompareOp::eNever;
        case CompareOp::Always:
            return vk::CompareOp::eAlways;
        case CompareOp::Less:
            return vk::CompareOp::eLess;
        case CompareOp::Equal:
            return vk::CompareOp::eEqual;
        case CompareOp::LessEqual:
            return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater:
            return vk::CompareOp::eGreater;
        case CompareOp::NotEqual:
            return vk::CompareOp::eNotEqual;
        case CompareOp::GreaterEqual:
            return vk::CompareOp::eGreaterOrEqual;
        default:
            FE_UNREACHABLE("Invalid CompareOp");
            return static_cast<vk::CompareOp>(-1);
        }
    }

    inline vk::PolygonMode VKConvert(PolygonMode source)
    {
        switch (source)
        {
        case PolygonMode::Fill:
            return vk::PolygonMode::eFill;
        case PolygonMode::Line:
            return vk::PolygonMode::eLine;
        case PolygonMode::Point:
            return vk::PolygonMode::ePoint;
        default:
            FE_UNREACHABLE("Invalid PolygonMode");
            return static_cast<vk::PolygonMode>(-1);
        }
    }

    inline vk::CullModeFlags VKConvert(CullingModeFlags source)
    {
        auto result = static_cast<vk::CullModeFlags>(0);
        if ((source & CullingModeFlags::Front) != CullingModeFlags::None)
        {
            result |= vk::CullModeFlagBits::eFront;
        }
        if ((source & CullingModeFlags::Back) != CullingModeFlags::None)
        {
            result |= vk::CullModeFlagBits::eBack;
        }
        return result;
    }

    inline vk::PrimitiveTopology VKConvert(PrimitiveTopology source)
    {
        switch (source)
        {
        case PrimitiveTopology::PointList:
            return vk::PrimitiveTopology::ePointList;
        case PrimitiveTopology::LineList:
            return vk::PrimitiveTopology::eLineList;
        case PrimitiveTopology::LineStrip:
            return vk::PrimitiveTopology::eLineStrip;
        case PrimitiveTopology::TriangleList:
            return vk::PrimitiveTopology::eTriangleList;
        case PrimitiveTopology::TriangleStrip:
            return vk::PrimitiveTopology::eTriangleStrip;
        default:
            FE_UNREACHABLE("Invalid PrimitiveTopology");
            return static_cast<vk::PrimitiveTopology>(-1);
        }
    }

    inline vk::VertexInputRate VKConvert(InputStreamRate source)
    {
        switch (source)
        {
        case InputStreamRate::PerVertex:
            return vk::VertexInputRate::eVertex;
        case InputStreamRate::PerInstance:
            return vk::VertexInputRate::eInstance;
        default:
            FE_UNREACHABLE("Invalid InputStreamRate");
            return static_cast<vk::VertexInputRate>(-1);
        }
    }

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
        vk::PipelineMultisampleStateCreateInfo result{};
        result.sampleShadingEnable   = false;
        result.rasterizationSamples  = vk::SampleCountFlagBits::e1;
        result.minSampleShading      = 1.0f;
        result.pSampleMask           = nullptr;
        result.alphaToCoverageEnable = false;
        result.alphaToOneEnable      = false;
        return result;
    }

    vk::PipelineDepthStencilStateCreateInfo VKGraphicsPipeline::BuildDepthState() const
    {
        vk::PipelineDepthStencilStateCreateInfo result{};
        result.depthTestEnable  = m_Desc.DepthStencil.DepthTestEnabled;
        result.depthWriteEnable = m_Desc.DepthStencil.DepthWriteEnabled;
        result.depthCompareOp   = VKConvert(m_Desc.DepthStencil.DepthCompareOp);
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
} // namespace FE::GPU
