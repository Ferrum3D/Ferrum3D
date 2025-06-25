#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/ImageFormat.h>
#include <Graphics/Core/Vulkan/PipelineStates.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/ShaderReflection.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    GraphicsPipeline::GraphicsPipeline(Core::Device* device)
    {
        m_device = device;
    }


    void GraphicsPipeline::InitInternal(const GraphicsPipelineInitContext& context)
    {
        FE_PROFILER_ZONE();

        namespace Limits = Core::Limits::Pipeline;

        m_desc = context.m_desc;

        const bool isMeshShadingPipeline = m_desc.m_shaders[festd::to_underlying(Core::ShaderStage::kMesh)] != Env::Name::kEmpty;
        if (isMeshShadingPipeline)
            FE_Assert(m_desc.m_inputLayout.IsEmpty());

        if (m_desc.m_shaders[festd::to_underlying(Core::ShaderStage::kAmplification)])
            FE_Assert(isMeshShadingPipeline);

        if (m_desc.m_shaders[festd::to_underlying(Core::ShaderStage::kVertex)])
            FE_Assert(!isMeshShadingPipeline);

        FE_Assert(m_desc.m_shaders[festd::to_underlying(Core::ShaderStage::kPixel)]);

        VkFormat rtvFormats[Core::Limits::Pipeline::kMaxColorAttachments];
        for (int32_t rtIndex = 0; rtIndex < m_desc.m_renderTargetCount; ++rtIndex)
            rtvFormats[rtIndex] = Translate(m_desc.m_rtvFormats[rtIndex]);

        VkFormat dsvFormat = Translate(m_desc.m_dsvFormat);

        VkPipelineRenderingCreateInfoKHR renderingCI = {};
        renderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        renderingCI.colorAttachmentCount = m_desc.m_renderTargetCount;
        renderingCI.pColorAttachmentFormats = rtvFormats;
        renderingCI.depthAttachmentFormat = dsvFormat;
        renderingCI.stencilAttachmentFormat = dsvFormat;

        VkGraphicsPipelineCreateInfo pipelineCI = {};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.pNext = &renderingCI;

        constexpr uint32_t kMaxShaderCount = festd::to_underlying(Core::ShaderStage::kGraphicsCount);

        festd::array<Core::ShaderHandle, kMaxShaderCount> shaderHandles;
        festd::fixed_vector<VkPipelineShaderStageCreateInfo, kMaxShaderCount> shaderStages;
        festd::fixed_vector<WaitGroup*, kMaxShaderCount> shaderWaitGroups;
        for (uint32_t shaderStageIndex = 0; shaderStageIndex < festd::size(m_desc.m_shaders); ++shaderStageIndex)
        {
            const Env::Name shaderName = m_desc.m_shaders[shaderStageIndex];
            if (!shaderName.IsValid())
                continue;

            const Core::ShaderHandle shaderHandle = context.m_shaderLibrary->GetShader(shaderName, context.m_defines);
            shaderHandles[shaderStageIndex] = shaderHandle;

            WaitGroup* waitGroup = context.m_shaderLibrary->GetCompletionWaitGroup(shaderHandle);
            shaderWaitGroups.push_back(waitGroup);
        }

        uint32_t specializationConstantOffset = 0;
        festd::fixed_vector<VkSpecializationMapEntry, Limits::kMaxSpecializationConstants> specializationMapEntries;
        festd::fixed_vector<VkSpecializationInfo, kMaxShaderCount> specializationInfos;
        festd::fixed_vector<int32_t, Limits::kMaxSpecializationConstants> specializationData;

        WaitGroup::WaitAll(shaderWaitGroups);

        festd::fixed_vector<const Core::ShaderReflection*, kMaxShaderCount> shaderReflections;
        for (uint32_t shaderStageIndex = 0; shaderStageIndex < festd::size(m_desc.m_shaders); ++shaderStageIndex)
        {
            const Env::Name shaderName = m_desc.m_shaders[shaderStageIndex];
            if (!shaderName.IsValid())
                continue;

            const Core::ShaderHandle shaderHandle = shaderHandles[shaderStageIndex];
            const ShaderModuleInfo shaderModuleInfo = context.m_shaderLibrary->GetShaderModule(shaderHandle);
            if (shaderModuleInfo.m_shaderModule == VK_NULL_HANDLE)
            {
                context.m_logger->LogError("Failed to create graphics pipeline: shader {} was not valid", shaderName);
                m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
                return;
            }

            ShaderReflection* reflection = context.m_shaderLibrary->GetReflection(shaderHandle);
            shaderReflections.push_back(reflection);

            VkPipelineShaderStageCreateInfo& shaderStageCI = shaderStages.push_back();
            shaderStageCI = {};
            shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCI.module = shaderModuleInfo.m_shaderModule;
            shaderStageCI.pName = shaderModuleInfo.m_entryPoint;
            shaderStageCI.stage = Translate(static_cast<Core::ShaderStage>(shaderStageIndex));

            uint32_t specializationConstantCount = 0;
            for (const Core::ShaderSpecializationConstant constant : context.m_specializationConstants)
            {
                const uint32_t constantIndex = festd::find_index(reflection->GetSpecializationConstantNames(), constant.m_name);
                if (constantIndex == kInvalidIndex)
                    continue;

                VkSpecializationMapEntry& entry = specializationMapEntries.push_back();
                entry.constantID = constantIndex;
                entry.offset = specializationConstantOffset;
                entry.size = sizeof(constant.m_value);
                specializationConstantOffset += sizeof(int32_t);
                specializationData.push_back(constant.m_value);
            }

            if (specializationConstantCount > 0)
            {
                VkSpecializationInfo& specializationInfo = specializationInfos.push_back();
                specializationInfo.mapEntryCount = specializationConstantCount;
                specializationInfo.pMapEntries =
                    specializationMapEntries.data() + specializationMapEntries.size() - specializationConstantCount;
                specializationInfo.dataSize = specializationConstantCount * sizeof(int32_t);
                specializationInfo.pData = specializationData.data() + specializationData.size() - specializationConstantCount;

                if (!specializationMapEntries.empty())
                    shaderStageCI.pSpecializationInfo = &specializationInfo;
            }
        }

        pipelineCI.pStages = shaderStages.data();
        pipelineCI.stageCount = shaderStages.size();

        constexpr VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_BLEND_CONSTANTS,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pDynamicStates = dynamicStates;
        dynamicStateCI.dynamicStateCount = festd::size(dynamicStates);
        pipelineCI.pDynamicState = &dynamicStateCI;

        if (isMeshShadingPipeline)
        {
            pipelineCI.pVertexInputState = nullptr;
            pipelineCI.pInputAssemblyState = nullptr;
        }
        else
        {
            festd::fixed_vector<VkVertexInputBindingDescription, Limits::kMaxVertexStreams> vertexBindings;
            festd::fixed_vector<VkVertexInputAttributeDescription, Limits::kMaxStreamChannels> vertexAttributes;

            VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
            vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            Bit::Traverse(m_desc.m_inputLayout.m_activeChannelsMask, [&](const uint32_t channelIndex) {
                const Core::InputStreamLayout& layout = m_desc.m_inputLayout;
                const Core::InputStreamChannelDesc& channel = layout.m_channels[channelIndex];

                uint32_t bindingIndex = festd::find_index_if(vertexBindings, [&](const VkVertexInputBindingDescription& binding) {
                    return binding.binding == channel.m_streamIndex;
                });

                if (bindingIndex == kInvalidIndex)
                {
                    bindingIndex = vertexBindings.size();

                    const bool isPerInstanceStream = layout.m_perInstanceStreamsMask & (1 << channel.m_streamIndex);

                    VkVertexInputBindingDescription& newBinding = vertexBindings.push_back();
                    newBinding = {};
                    newBinding.binding = channel.m_streamIndex;
                    newBinding.inputRate = isPerInstanceStream ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
                    newBinding.stride = 0;
                }

                const Core::FormatInfo formatInfo{ channel.m_format };

                VkVertexInputBindingDescription& binding = vertexBindings[bindingIndex];
                binding.stride = Math::Max(binding.stride, channel.m_offset + formatInfo.m_texelByteSize);

                const Core::ShaderHandle vsHandle = shaderHandles[festd::to_underlying(Core::ShaderStage::kVertex)];
                const ShaderReflection* vsReflection = context.m_shaderLibrary->GetReflection(vsHandle);

                const Core::ShaderSemantic channelSemantic{ channel.m_shaderSemanticName, channel.m_shaderSemanticIndex };

                VkVertexInputAttributeDescription& attribute = vertexAttributes.push_back();
                attribute = {};
                attribute.location = vsReflection->GetInputAttributeLocation(channelSemantic.ToName());
                attribute.binding = channel.m_streamIndex;
                attribute.format = Translate(formatInfo.m_format);
                attribute.offset = channel.m_offset;
            });

            vertexInputCI.pVertexBindingDescriptions = vertexBindings.data();
            vertexInputCI.vertexBindingDescriptionCount = vertexBindings.size();
            vertexInputCI.pVertexAttributeDescriptions = vertexAttributes.data();
            vertexInputCI.vertexAttributeDescriptionCount = vertexAttributes.size();
            pipelineCI.pVertexInputState = &vertexInputCI;

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {};
            inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyCI.topology = Translate(m_desc.m_inputLayout.m_topology);
            pipelineCI.pInputAssemblyState = &inputAssemblyCI;
        }

        VkPipelineViewportStateCreateInfo viewportCI = {};
        viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportCI.viewportCount = 1;
        viewportCI.scissorCount = 1;
        pipelineCI.pViewportState = &viewportCI;

        VkPipelineRasterizationStateCreateInfo rasterCI = {};
        rasterCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterCI.lineWidth = 1.0f;
        rasterCI.polygonMode = Translate(m_desc.m_rasterization.m_polyMode);
        rasterCI.cullMode = Translate(m_desc.m_rasterization.m_cullMode);
        rasterCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
        pipelineCI.pRasterizationState = &rasterCI;

        VkPipelineMultisampleStateCreateInfo multisamplingCI{};
        multisamplingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCI.sampleShadingEnable = VK_FALSE;
        multisamplingCI.rasterizationSamples = GetVKSampleCountFlags(m_desc.m_sampleCount);
        pipelineCI.pMultisampleState = &multisamplingCI;

        VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
        depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilCI.depthTestEnable = m_desc.m_depthStencil.m_depthTestEnabled;
        depthStencilCI.stencilTestEnable = m_desc.m_depthStencil.m_stencilTestEnabled;
        depthStencilCI.depthWriteEnable = m_desc.m_depthStencil.m_depthWriteEnabled;
        depthStencilCI.depthCompareOp = Translate(m_desc.m_depthStencil.m_depthCompareOp);
        depthStencilCI.depthBoundsTestEnable = false;
        depthStencilCI.minDepthBounds = 0.0f;
        depthStencilCI.maxDepthBounds = 1.0f;
        pipelineCI.pDepthStencilState = &depthStencilCI;

        festd::fixed_vector<VkPipelineColorBlendAttachmentState, Limits::kMaxColorAttachments> colorBlendAttachments;
        for (int32_t rtIndex = 0; rtIndex < m_desc.m_renderTargetCount; ++rtIndex)
        {
            const int32_t blendStateIndex = m_desc.m_colorBlend.m_enableIndependentBlend ? rtIndex : 0;
            const Core::TargetColorBlending src = m_desc.m_colorBlend.m_targetBlendStates[blendStateIndex];

            VkPipelineColorBlendAttachmentState& dst = colorBlendAttachments.push_back();
            dst = {};
            dst.blendEnable = src.m_blendEnabled;
            dst.colorWriteMask = Translate(src.m_colorWriteFlags);
            dst.srcColorBlendFactor = Translate(src.m_sourceFactor);
            dst.srcAlphaBlendFactor = Translate(src.m_sourceAlphaFactor);
            dst.dstColorBlendFactor = Translate(src.m_destinationFactor);
            dst.dstAlphaBlendFactor = Translate(src.m_destinationAlphaFactor);
            dst.colorBlendOp = Translate(src.m_blendOp);
            dst.alphaBlendOp = Translate(src.m_alphaBlendOp);
        }

        VkPipelineColorBlendStateCreateInfo colorBlendCI{};
        colorBlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendCI.logicOpEnable = VK_FALSE;
        colorBlendCI.logicOp = VK_LOGIC_OP_COPY;
        colorBlendCI.attachmentCount = colorBlendAttachments.size();
        colorBlendCI.pAttachments = colorBlendAttachments.data();
        pipelineCI.pColorBlendState = &colorBlendCI;

        festd::fixed_vector<VkPushConstantRange, Limits::kMaxShaderResourceGroups> pushConstantRanges;
        for (const Core::ShaderReflection* reflection : shaderReflections)
        {
            for (const Core::ShaderRootConstant& rootConstants : reflection->GetRootConstants())
            {
                if (rootConstants.m_byteSize > 0)
                {
                    // HACK: assume VK_SHADER_STAGE_ALL always works.
                    if (!pushConstantRanges.empty())
                        break;

                    VkPushConstantRange& range = pushConstantRanges.push_back();
                    range.size = rootConstants.m_byteSize;
                    range.offset = rootConstants.m_offset;
                    range.stageFlags = VK_SHADER_STAGE_ALL;
                }
            }
        }

        m_descriptorSetLayout = context.m_bindlessSetLayout;

        const VkDevice device = NativeCast(m_device);

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.pSetLayouts = &m_descriptorSetLayout;
        layoutCI.setLayoutCount = 1;

        if (!pushConstantRanges.empty())
        {
            layoutCI.pushConstantRangeCount = pushConstantRanges.size();
            layoutCI.pPushConstantRanges = pushConstantRanges.data();
        }

        if (const VkResult result = vkCreatePipelineLayout(device, &layoutCI, VK_NULL_HANDLE, &m_layout); result != VK_SUCCESS)
        {
            context.m_logger->LogError("Failed to create pipeline layout: {}", VKResultToString(result));
            m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
            return;
        }

        pipelineCI.layout = m_layout;
        if (const VkResult result =
                vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, VK_NULL_HANDLE, &m_nativePipeline);
            result != VK_SUCCESS)
        {
            context.m_logger->LogError("Failed to create graphics pipeline: {}", VKResultToString(result));
            m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
            return;
        }

        m_status.store(Core::PipelineStatus::kReady, std::memory_order_release);
    }


    GraphicsPipeline::~GraphicsPipeline()
    {
        if (IsReady())
        {
            const VkDevice device = NativeCast(m_device);
            vkDestroyPipeline(device, m_nativePipeline, nullptr);
            vkDestroyPipelineLayout(device, m_layout, nullptr);
        }

        m_status.store(Core::PipelineStatus::kNotReady, std::memory_order_release);
    }
} // namespace FE::Graphics::Vulkan
