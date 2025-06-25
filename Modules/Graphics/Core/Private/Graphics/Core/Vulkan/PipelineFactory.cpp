#include <FeCore/DI/Activator.h>
#include <FeCore/Jobs/Job.h>
#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>

namespace FE::Graphics::Vulkan
{
    template<class TPipeline>
    struct PipelineFactory::AsyncCompilationJob final : public Job
    {
        void Execute() override
        {
            FE_PROFILER_ZONE();

            m_pipeline->InitInternal(m_context);
            Memory::Delete(&m_factory->m_jobPool, this);
        }

        PipelineFactory* m_factory = nullptr;
        TPipeline* m_pipeline = nullptr;
        typename TPipeline::InitContext m_context;
    };


    PipelineFactory::PipelineFactory(Core::Device* device, BindlessManager* bindlessManager, IJobSystem* jobSystem,
                                     Logger* logger)
        : m_graphicsPipelinePool("GraphicsPipelinePool", sizeof(GraphicsPipeline))
        , m_computePipelinePool("ComputePipelinePool", sizeof(ComputePipeline))
        , m_jobPool("PipelineAsyncCompilationJobPool",
                    Math::Max(sizeof(AsyncCompilationJob<GraphicsPipeline>), sizeof(AsyncCompilationJob<ComputePipeline>)))
        , m_bindlessManager(bindlessManager)
        , m_jobSystem(jobSystem)
        , m_logger(logger)
    {
        FE_PROFILER_ZONE();

        m_device = device;
        SetImmediateDestroyPolicy();

        m_logger->LogTrace("Creating Pipeline Factory");

        VkPipelineCacheCreateInfo pipelineCacheCI{};
        pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCI.initialDataSize = 0;
        pipelineCacheCI.pInitialData = nullptr;

        VerifyVulkan(vkCreatePipelineCache(NativeCast(device), &pipelineCacheCI, nullptr, &m_pipelineCache));

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

        auto* pipeline = DI::New<GraphicsPipeline>(&m_graphicsPipelinePool).value();
        pipeline->AddRef();
        m_graphicsPipelinesMap[hash] = pipeline;
        lock.unlock();

        const Rc waitGroup = WaitGroup::Create();

        auto* job = Memory::New<AsyncCompilationJob<GraphicsPipeline>>(&m_jobPool);
        job->m_context.m_specializationConstants.resize(request.m_specializationConstants.size());
        Memory::Copy(festd::span(job->m_context.m_specializationConstants), request.m_specializationConstants);

        job->m_factory = this;
        job->m_pipeline = pipeline;
        job->m_context.m_defines = request.m_defines;
        job->m_context.m_desc = request.m_desc;
        job->m_context.m_pipelineCache = m_pipelineCache;
        job->m_context.m_shaderLibrary = m_shaderLibrary.Get();
        job->m_context.m_logger = m_logger;
        job->m_context.m_bindlessSetLayout = m_bindlessManager->GetDescriptorSetLayout();
        job->ScheduleBackground(m_jobSystem, waitGroup.Get(), JobPriority::kNormal);
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

        auto* pipeline = DI::New<ComputePipeline>(&m_computePipelinePool).value();
        pipeline->AddRef();
        m_computePipelinesMap[hash] = pipeline;
        lock.unlock();

        const Rc waitGroup = WaitGroup::Create();

        auto* job = Memory::New<AsyncCompilationJob<ComputePipeline>>(&m_jobPool);
        job->m_context.m_specializationConstants.resize(request.m_specializationConstants.size());
        Memory::Copy(festd::span(job->m_context.m_specializationConstants), request.m_specializationConstants);

        job->m_factory = this;
        job->m_pipeline = pipeline;
        job->m_context.m_defines = request.m_defines;
        job->m_context.m_desc = request.m_desc;
        job->m_context.m_pipelineCache = m_pipelineCache;
        job->m_context.m_shaderLibrary = m_shaderLibrary.Get();
        job->m_context.m_logger = m_logger;
        job->m_context.m_bindlessSetLayout = m_bindlessManager->GetDescriptorSetLayout();
        job->ScheduleBackground(m_jobSystem, waitGroup.Get(), JobPriority::kNormal);
        pipeline->SetCompletionWaitGroup(waitGroup.Get());
        return pipeline;
    }
} // namespace FE::Graphics::Vulkan
