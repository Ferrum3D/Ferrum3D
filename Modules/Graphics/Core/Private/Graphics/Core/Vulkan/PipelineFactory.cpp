#include <FeCore/DI/Activator.h>
#include <FeCore/Jobs/Job.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>

namespace FE::Graphics::Vulkan
{
    struct PipelineFactory::AsyncCompilationJob final : public Job
    {
        void Execute() override
        {
            FE_PROFILER_ZONE();

            m_pipeline->InitInternal(m_context);
            Memory::Delete(&m_factory->m_jobPool, this, sizeof(AsyncCompilationJob));
        }

        PipelineFactory* m_factory = nullptr;
        GraphicsPipeline* m_pipeline = nullptr;
        GraphicsPipelineInitContext m_context;
    };


    PipelineFactory::PipelineFactory(Core::Device* device, IJobSystem* jobSystem, Logger* logger)
        : m_graphicsPipelinePool("GraphicsPipelinePool", sizeof(GraphicsPipeline))
        , m_jobPool("PipelineAsyncCompilationJobPool", sizeof(AsyncCompilationJob))
        , m_jobSystem(jobSystem)
        , m_logger(logger)
    {
        FE_PROFILER_ZONE();

        m_device = device;

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
            pipeline->Release();

        if (m_pipelineCache)
            vkDestroyPipelineCache(NativeCast(m_device), m_pipelineCache, nullptr);
    }


    Core::GraphicsPipeline* PipelineFactory::CreateGraphicsPipeline(const Core::GraphicsPipelineRequest& request)
    {
        FE_PROFILER_ZONE();

        const uint64_t hash = request.GetHash();
        const auto it = m_graphicsPipelinesMap.find(hash);
        if (it != m_graphicsPipelinesMap.end())
            return it->second;

        GraphicsPipeline* pipeline = DI::New<GraphicsPipeline>(&m_graphicsPipelinePool).value();
        pipeline->AddRef();
        m_graphicsPipelinesMap[hash] = pipeline;

        const Rc waitGroup = WaitGroup::Create();

        AsyncCompilationJob* job = Memory::New<AsyncCompilationJob>(&m_jobPool);
        job->m_factory = this;
        job->m_pipeline = pipeline;
        job->m_context.m_defines.Init(request.m_defines);
        job->m_context.m_desc = request.m_desc;
        job->m_context.m_pipelineCache = m_pipelineCache;
        job->m_context.m_shaderLibrary = m_shaderLibrary.Get();
        job->m_context.m_logger = m_logger;
        job->ScheduleBackground(m_jobSystem, waitGroup.Get(), JobPriority::kNormal);
        pipeline->SetCompletionWaitGroup(waitGroup.Get());
        return pipeline;
    }
} // namespace FE::Graphics::Vulkan
