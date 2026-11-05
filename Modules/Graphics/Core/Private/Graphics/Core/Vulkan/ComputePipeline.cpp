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

        const Rc shader = context.m_shaderLibrary->GetVersion(shaderHandle);
        if (!shader)
        {
            Logger::LogError("Failed to create compute pipeline: shader {} was not valid", shaderName);
            m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
            return;
        }

        const Core::ShaderReflection* reflection = shader->GetReflection();

        VkPipelineShaderStageCreateInfo shaderStageCI = {};
        shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageCI.module = shader->GetModule();
        shaderStageCI.pName = shader->GetEntryPoint();

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

        VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroupSize{};
        subgroupSize.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        if (m_desc.m_waveLaneCount != 0)
        {
            const Core::ComputeCapabilities capabilities = m_device->GetComputeCapabilities();
            const uint32_t size = m_desc.m_waveLaneCount;
            const bool validSize =
                (size & (size - 1)) == 0 && size >= capabilities.m_minWaveLaneCount && size <= capabilities.m_maxWaveLaneCount;
            if (!capabilities.m_waveLaneCountControl || !validSize)
            {
                Logger::LogError("Unsupported compute subgroup size {}", size);
                m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
                return;
            }

            subgroupSize.requiredSubgroupSize = size;
            shaderStageCI.pNext = &subgroupSize;
        }

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
                m_rootConstantsByteSize = Math::Max(m_rootConstantsByteSize, rootConstants.m_byteSize);
            }
        }

        if (!pushConstantRanges.empty())
        {
            layoutCI.pushConstantRangeCount = pushConstantRanges.size();
            layoutCI.pPushConstantRanges = pushConstantRanges.data();
        }

        if (const VkResult result = vkCreatePipelineLayout(device, &layoutCI, VK_NULL_HANDLE, &m_layout); result != VK_SUCCESS)
        {
            Logger::LogError("Failed to create pipeline layout: {}", ResultToString(result));
            m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
            return;
        }

        pipelineCI.layout = m_layout;
        if (const VkResult result =
                vkCreateComputePipelines(device, context.m_pipelineCache, 1, &pipelineCI, VK_NULL_HANDLE, &m_nativePipeline);
            result != VK_SUCCESS)
        {
            Logger::LogError("Failed to create graphics pipeline: {}", ResultToString(result));
            m_status.store(Core::PipelineStatus::kError, std::memory_order_release);
            return;
        }

        m_status.store(Core::PipelineStatus::kReady, std::memory_order_release);
    }


    void ComputePipeline::SwapNative(ComputePipeline& other)
    {
        FE_Assert(other.m_desc.GetHash() == m_desc.GetHash());

        eastl::swap(m_layout, other.m_layout);
        eastl::swap(m_nativePipeline, other.m_nativePipeline);

        const Core::PipelineStatus status = m_status.load(std::memory_order_acquire);
        m_status.store(other.m_status.load(std::memory_order_acquire), std::memory_order_release);
        other.m_status.store(status, std::memory_order_release);
    }


    ComputePipeline::~ComputePipeline()
    {
        // A failed creation or a swapped-out version may own only some of the objects.
        // Swapped-out versions may still be used by submitted work, so the device destroys them once it completes.
        Device* device = ImplCast(m_device);
        device->Retire(m_nativePipeline);
        device->Retire(m_layout);

        m_status.store(Core::PipelineStatus::kNotReady, std::memory_order_release);
    }
} // namespace FE::Graphics::Vulkan
