#include <Core/DI/Activator.h>
#include <Core/Jobs/Jobs.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>

namespace FE::Graphics::Vulkan
{
    PipelineFactory::PipelineFactory(Core::Device* device, Core::DescriptorManager* bindlessManager)
        : m_descriptorManager(ImplCast(bindlessManager))
    {
        FE_PROFILER_ZONE();

        m_device = device;
        SetImmediateDestroyPolicy();

        Logger::LogTrace("Creating Pipeline Factory");

        VkPipelineCacheCreateInfo pipelineCacheCI{};
        pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCI.initialDataSize = 0;
        pipelineCacheCI.pInitialData = nullptr;

        VerifyVk(vkCreatePipelineCache(NativeCast(device), &pipelineCacheCI, nullptr, &m_pipelineCache));

        m_shaderLibrary = DI::DefaultNew<ShaderLibrary>().value();
    }


    PipelineFactory::~PipelineFactory()
    {
        for (const auto [hash, pipeline] : m_graphicsPipelinesMap)
        {
            FE_AssertDebug(pipeline->GetRefCount() == 1);
            pipeline->SetImmediateDestroyPolicy();
            pipeline->Release();
        }

        for (const auto [hash, pipeline] : m_computePipelinesMap)
        {
            FE_AssertDebug(pipeline->GetRefCount() == 1);
            pipeline->SetImmediateDestroyPolicy();
            pipeline->Release();
        }

        if (m_pipelineCache)
            vkDestroyPipelineCache(NativeCast(m_device), m_pipelineCache, nullptr);
    }


    Core::GraphicsPipeline* PipelineFactory::CreateGraphicsPipeline(const Core::GraphicsPipelineRequest& request)
    {
        FE_PROFILER_ZONE();

        std::unique_lock lock{ m_lock };

        const uint64_t hash = request.GetHash();
        const auto it = m_graphicsPipelinesMap.find(hash);
        if (it != m_graphicsPipelinesMap.end())
            return it->second;

        auto* pipeline = DI::DefaultNew<GraphicsPipeline>().value();
        pipeline->AddRef();
        m_graphicsPipelinesMap[hash] = pipeline;
        lock.unlock();

        GraphicsPipeline::InitContext context;
        context.m_defines = request.m_defines;
        context.m_desc = request.m_desc;
        context.m_pipelineCache = m_pipelineCache;
        context.m_shaderLibrary = m_shaderLibrary.Get();
        context.m_bindlessSetLayout = m_descriptorManager->GetDescriptorSetLayout();
        context.m_specializationConstants.resize(request.m_specializationConstants.size());
        Memory::Copy(festd::span(context.m_specializationConstants), request.m_specializationConstants);

        const Rc waitGroup = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, pipeline, context = std::move(context)] {
                pipeline->InitInternal(context);
            },
            waitGroup.Get());

        pipeline->SetCompletionWaitGroup(waitGroup.Get());
        return pipeline;
    }


    Core::ComputePipeline* PipelineFactory::CreateComputePipeline(const Core::ComputePipelineRequest& request)
    {
        FE_PROFILER_ZONE();

        std::unique_lock lock{ m_lock };

        const uint64_t hash = request.GetHash();
        const auto it = m_computePipelinesMap.find(hash);
        if (it != m_computePipelinesMap.end())
            return it->second;

        auto* pipeline = DI::DefaultNew<ComputePipeline>().value();
        pipeline->AddRef();
        m_computePipelinesMap[hash] = pipeline;
        lock.unlock();

        ComputePipeline::InitContext context;
        context.m_defines = request.m_defines;
        context.m_desc = request.m_desc;
        context.m_pipelineCache = m_pipelineCache;
        context.m_shaderLibrary = m_shaderLibrary.Get();
        context.m_bindlessSetLayout = m_descriptorManager->GetDescriptorSetLayout();
        context.m_specializationConstants.resize(request.m_specializationConstants.size());
        Memory::Copy(festd::span(context.m_specializationConstants), request.m_specializationConstants);

        const Rc waitGroup = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, pipeline, context = std::move(context)] {
                pipeline->InitInternal(context);
            },
            waitGroup.Get());

        pipeline->SetCompletionWaitGroup(waitGroup.Get());
        return pipeline;
    }
} // namespace FE::Graphics::Vulkan
