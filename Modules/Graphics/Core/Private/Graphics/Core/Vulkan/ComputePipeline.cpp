#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    ComputePipeline::ComputePipeline(Core::Device* device)
    {
        m_device = device;
    }


    void ComputePipeline::InitInternal(const ComputePipelineInitContext& context)
    {
        FE_PROFILER_ZONE();

        namespace Limits = Core::Limits::Pipeline;

        m_desc = context.m_desc;

        VkComputePipelineCreateInfo pipelineCI = {};
        pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

        FE_Assert(m_desc.m_shader.IsValid());

        const Env::Name shaderName = m_desc.m_shader;

        const Core::ShaderHandle shaderHandle = context.m_shaderLibrary->GetShader(shaderName, context.m_defines);
        WaitGroup* waitGroup = context.m_shaderLibrary->GetCompletionWaitGroup(shaderHandle);
        waitGroup->Wait();

        const Core::ShaderReflection* reflection = context.m_shaderLibrary->GetReflection(shaderHandle);
        const ShaderModuleInfo shaderModuleInfo = context.m_shaderLibrary->GetShaderModule(shaderHandle);

        VkPipelineShaderStageCreateInfo shaderStageCI = {};
        shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageCI.module = shaderModuleInfo.m_shaderModule;
        shaderStageCI.pName = shaderModuleInfo.m_entryPoint;

        uint32_t specializationConstantOffset = 0;
        festd::fixed_vector<VkSpecializationMapEntry, Limits::kMaxSpecializationConstants> specializationMapEntries;
        festd::fixed_vector<int32_t, Limits::kMaxSpecializationConstants> specializationData;
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

        VkSpecializationInfo specializationInfo = {};
        specializationInfo.mapEntryCount = specializationMapEntries.size();
        specializationInfo.pMapEntries = specializationMapEntries.data();
        specializationInfo.dataSize = specializationData.size() * sizeof(int32_t);
        specializationInfo.pData = specializationData.data();

        if (!specializationMapEntries.empty())
            shaderStageCI.pSpecializationInfo = &specializationInfo;

        pipelineCI.stage = shaderStageCI;

        m_descriptorSetLayout = context.m_bindlessSetLayout;

        const VkDevice device = NativeCast(m_device);

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.pSetLayouts = &m_descriptorSetLayout;
        layoutCI.setLayoutCount = 1;

        festd::fixed_vector<VkPushConstantRange, Limits::kMaxShaderResourceGroups> pushConstantRanges;
        for (const Core::ShaderRootConstant& rootConstants : reflection->GetRootConstants())
        {
            if (rootConstants.m_byteSize > 0)
            {
                VkPushConstantRange& range = pushConstantRanges.push_back();
                range.size = rootConstants.m_byteSize;
                range.offset = rootConstants.m_offset;
                range.stageFlags = VK_SHADER_STAGE_ALL;
            }
        }

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
                vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, VK_NULL_HANDLE, &m_nativePipeline);
            result != VK_SUCCESS)
        {
            context.m_logger->LogError("Failed to create graphics pipeline: {}", VKResultToString(result));
            m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
            return;
        }

        m_status.store(Core::PipelineStatus::kReady, std::memory_order_release);
    }


    ComputePipeline::~ComputePipeline()
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
