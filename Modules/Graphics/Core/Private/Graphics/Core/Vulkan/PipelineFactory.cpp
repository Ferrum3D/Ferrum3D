#include <Core/IO/FileStream.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Time/BaseTime.h>
#include <Graphics/Core/Common/ShaderCache.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/ShaderCompilerDXC.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/PipelineFactory.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        //! How long shader sources must stay unchanged before a reload starts.
        constexpr double kShaderReloadDelaySeconds = 0.2;


        //! Read a pipeline cache saved for the same adapter and driver, or return empty data.
        festd::vector<std::byte> LoadPipelineCacheData(const IO::Path& path, const VkPhysicalDeviceProperties& adapterProperties)
        {
            festd::vector<std::byte> data;

            auto stream = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
            if (!stream)
                return data;

            data.resize(static_cast<uint32_t>(stream.value()->Length()));
            if (stream.value()->ReadToBuffer(data.data(), data.size()) != data.size())
                return {};

            // Drivers are required to reject foreign caches, but checking the header avoids relying on that.
            VkPipelineCacheHeaderVersionOne header;
            if (data.size() < sizeof(header))
                return {};

            memcpy(&header, data.data(), sizeof(header));
            if (header.headerVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
                return {};

            if (header.vendorID != adapterProperties.vendorID || header.deviceID != adapterProperties.deviceID)
                return {};

            if (memcmp(header.pipelineCacheUUID, adapterProperties.pipelineCacheUUID, VK_UUID_SIZE) != 0)
                return {};

            return data;
        }


        void SavePipelineCacheData(const VkDevice device, const VkPipelineCache pipelineCache, const IO::Path& path)
        {
            FE_PROFILER_ZONE();

            size_t dataSize = 0;
            if (vkGetPipelineCacheData(device, pipelineCache, &dataSize, nullptr) != VK_SUCCESS || dataSize == 0)
                return;

            festd::vector<std::byte> data(static_cast<uint32_t>(dataSize));
            if (vkGetPipelineCacheData(device, pipelineCache, &dataSize, data.data()) != VK_SUCCESS)
                return;

            if (!Core::WriteShaderCacheFile(path, festd::span{ data.data(), static_cast<uint32_t>(dataSize) }))
                Logger::LogWarning("Failed to save pipeline cache '{}'", path);
        }
    } // namespace


    PipelineFactory::PipelineFactory(Core::Device* device, Core::DescriptorManager* bindlessManager)
        : m_descriptorManager(ImplCast(bindlessManager))
    {
        FE_PROFILER_ZONE();

        m_device = device;
        SetImmediateDestroyPolicy();

        Logger::LogTrace("Creating Pipeline Factory");

        const VkPhysicalDeviceProperties& adapterProperties = ImplCast(device)->GetAdapterProperties();
        m_pipelineCachePath = Core::GetShaderCacheDirectory()
            / Fmt::FixedFormat("PipelineCache_{}_{}.bin", adapterProperties.vendorID, adapterProperties.deviceID);

        const festd::vector<std::byte> initialData = LoadPipelineCacheData(m_pipelineCachePath, adapterProperties);

        VkPipelineCacheCreateInfo pipelineCacheCI{};
        pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCI.initialDataSize = initialData.size();
        pipelineCacheCI.pInitialData = initialData.data();

        VerifyVk(vkCreatePipelineCache(NativeCast(device), &pipelineCacheCI, nullptr, &m_pipelineCache));

        m_shaderCompiler = Memory::DefaultNew<Core::ShaderCompilerDXC>();
        m_shaderLibrary = Memory::DefaultNew<ShaderLibrary>(device, m_shaderCompiler.Get());

        ImplCast(device)->RegisterPipelineFactory(this);
    }


    PipelineFactory::~PipelineFactory()
    {
        ImplCast(m_device)->UnregisterPipelineFactory(this);

        if (m_shaderReloadCompletion)
            m_shaderReloadCompletion->Wait();

        m_shaderReload = {};

        for (const auto& [hash, entry] : m_graphicsPipelinesMap)
        {
            FE_AssertDebug(entry.m_pipeline->GetRefCount() == 1);
            entry.m_pipeline->SetImmediateDestroyPolicy();
            entry.m_pipeline->Release();
        }

        for (const auto& [hash, entry] : m_computePipelinesMap)
        {
            FE_AssertDebug(entry.m_pipeline->GetRefCount() == 1);
            entry.m_pipeline->SetImmediateDestroyPolicy();
            entry.m_pipeline->Release();
        }

        if (m_pipelineCache)
        {
            SavePipelineCacheData(NativeCast(m_device), m_pipelineCache, m_pipelineCachePath);
            vkDestroyPipelineCache(NativeCast(m_device), m_pipelineCache, nullptr);
        }
    }


    Core::ShaderLibrary* PipelineFactory::GetShaderLibrary() const
    {
        return m_shaderLibrary.Get();
    }


    Core::GraphicsPipeline* PipelineFactory::CreateGraphicsPipeline(const Core::GraphicsPipelineRequest& request)
    {
        FE_PROFILER_ZONE();

        std::unique_lock lock{ m_lock };

        const uint64_t hash = request.GetHash();
        const auto it = m_graphicsPipelinesMap.find(hash);
        if (it != m_graphicsPipelinesMap.end())
            return it->second.m_pipeline;

        auto* pipeline = Memory::DefaultNew<GraphicsPipeline>(m_device);
        pipeline->AddRef();

        GraphicsPipeline::InitContext context;
        context.m_defines = request.m_defines;
        context.m_desc = request.m_desc;
        context.m_pipelineCache = m_pipelineCache;
        context.m_shaderLibrary = m_shaderLibrary.Get();
        context.m_bindlessSetLayout = m_descriptorManager->GetDescriptorSetLayout();
        context.m_specializationConstants.resize(request.m_specializationConstants.size());
        Memory::Copy(festd::span(context.m_specializationConstants), request.m_specializationConstants);

        // The context is kept to rebuild the pipeline when its shaders are reloaded.
        PipelineEntry<GraphicsPipeline>& entry = m_graphicsPipelinesMap[hash];
        entry.m_pipeline = pipeline;
        entry.m_context = context;
        lock.unlock();

        const Rc waitGroup = WaitGroup::Create();
        Jobs::DispatchBackground(
            [pipeline, context = std::move(context)] {
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
            return it->second.m_pipeline;

        auto* pipeline = Memory::DefaultNew<ComputePipeline>(m_device);
        pipeline->AddRef();

        ComputePipeline::InitContext context;
        context.m_defines = request.m_defines;
        context.m_desc = request.m_desc;
        context.m_pipelineCache = m_pipelineCache;
        context.m_shaderLibrary = m_shaderLibrary.Get();
        context.m_bindlessSetLayout = m_descriptorManager->GetDescriptorSetLayout();
        context.m_specializationConstants.resize(request.m_specializationConstants.size());
        Memory::Copy(festd::span(context.m_specializationConstants), request.m_specializationConstants);

        // The context is kept to rebuild the pipeline when its shader is reloaded.
        PipelineEntry<ComputePipeline>& entry = m_computePipelinesMap[hash];
        entry.m_pipeline = pipeline;
        entry.m_context = context;
        lock.unlock();

        const Rc waitGroup = WaitGroup::Create();
        Jobs::DispatchBackground(
            [pipeline, context = std::move(context)] {
                pipeline->InitInternal(context);
            },
            waitGroup.Get());

        pipeline->SetCompletionWaitGroup(waitGroup.Get());
        return pipeline;
    }


    void PipelineFactory::UpdateShaderHotReload()
    {
        if (!Build::IsDevelopment())
            return;

        FE_PROFILER_ZONE();

        if (m_shaderReloadCompletion)
        {
            if (!m_shaderReloadCompletion->IsSignaled())
                return;

            ApplyShaderReload();
            m_shaderReloadCompletion.Reset();
        }

        const uint32_t previousChangeCount = m_pendingChangedPaths.size();
        m_shaderCompiler->GetSourceCache()->PollChangedPaths(m_pendingChangedPaths);
        if (m_pendingChangedPaths.size() != previousChangeCount)
            m_lastChangeTicks = Platform::GetTicks();

        if (m_pendingChangedPaths.empty())
            return;

        // Editors often save a file in several steps, so wait until the directory has been quiet for a moment.
        const double secondsSinceChange =
            static_cast<double>(Platform::GetTicks() - m_lastChangeTicks) * Platform::GetSecondsPerTick();
        if (secondsSinceChange < kShaderReloadDelaySeconds)
            return;

        m_shaderReload = {};
        m_shaderReload.m_changedPaths = std::move(m_pendingChangedPaths);
        m_pendingChangedPaths.clear();

        m_shaderReloadCompletion = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this] {
                PrepareShaderReload();
            },
            m_shaderReloadCompletion.Get());
    }


    void PipelineFactory::PrepareShaderReload()
    {
        FE_PROFILER_ZONE();

        festd::vector<Env::Name> changedSources;
        const festd::span changedPaths{ m_shaderReload.m_changedPaths.data(), m_shaderReload.m_changedPaths.size() };
        m_shaderCompiler->GetSourceCache()->Refresh(changedPaths, changedSources);
        if (changedSources.empty())
            return;

        festd::vector<Core::ShaderHandle> shaders;
        m_shaderLibrary->FindDependentShaders(festd::span{ changedSources.data(), changedSources.size() }, shaders);
        if (shaders.empty())
            return;

        festd::vector<Core::ShaderHandle> recompiledShaders;
        RecompileShaders(festd::span{ shaders.data(), shaders.size() }, recompiledShaders);
        if (!recompiledShaders.empty())
            BuildReplacementPipelines(festd::span{ recompiledShaders.data(), recompiledShaders.size() });
    }


    void PipelineFactory::RecompileShaders(const festd::span<const Core::ShaderHandle> shaders,
                                           festd::vector<Core::ShaderHandle>& recompiled)
    {
        FE_PROFILER_ZONE();

        festd::vector<uint8_t> isRecompiled(shaders.size(), 0);

        const Rc completion = WaitGroup::Create(shaders.size());
        for (uint32_t index = 0; index < shaders.size(); ++index)
        {
            Jobs::DispatchBackground(
                [this, shader = shaders[index], result = &isRecompiled[index]] {
                    *result = m_shaderLibrary->Recompile(shader) ? 1 : 0;
                },
                completion.Get());
        }

        completion->Wait();

        for (uint32_t index = 0; index < shaders.size(); ++index)
        {
            if (isRecompiled[index] != 0)
                recompiled.push_back(shaders[index]);
        }

        m_shaderReload.m_recompiledShaderCount = recompiled.size();
        m_shaderReload.m_failedShaderCount = shaders.size() - recompiled.size();
    }


    void PipelineFactory::BuildReplacementPipelines(const festd::span<const Core::ShaderHandle> recompiledShaders)
    {
        FE_PROFILER_ZONE();

        festd::vector<GraphicsPipeline::InitContext> graphicsContexts;
        festd::vector<ComputePipeline::InitContext> computeContexts;
        {
            std::lock_guard lock{ m_lock };

            for (const auto& [hash, entry] : m_graphicsPipelinesMap)
            {
                // Pipelines still being created are skipped. Most of them read the new shader versions already, and the
                // others are rebuilt by the next reload that touches their shaders.
                if (!entry.m_pipeline->GetCompletionWaitGroup()->IsSignaled())
                    continue;

                const festd::span shaderNames{ entry.m_context.m_desc.m_shaders };
                if (!UsesAnyShader(shaderNames, entry.m_context.m_defines, recompiledShaders))
                    continue;

                m_shaderReload.m_graphicsReplacements.push_back({ entry.m_pipeline, nullptr });
                graphicsContexts.push_back(entry.m_context);
            }

            for (const auto& [hash, entry] : m_computePipelinesMap)
            {
                if (!entry.m_pipeline->GetCompletionWaitGroup()->IsSignaled())
                    continue;

                const festd::span shaderNames{ &entry.m_context.m_desc.m_shader, 1 };
                if (!UsesAnyShader(shaderNames, entry.m_context.m_defines, recompiledShaders))
                    continue;

                m_shaderReload.m_computeReplacements.push_back({ entry.m_pipeline, nullptr });
                computeContexts.push_back(entry.m_context);
            }
        }

        const uint32_t replacementCount = graphicsContexts.size() + computeContexts.size();
        if (replacementCount == 0)
            return;

        const Rc completion = WaitGroup::Create(replacementCount);
        for (uint32_t index = 0; index < graphicsContexts.size(); ++index)
        {
            Rc replacement = Memory::DefaultNew<GraphicsPipeline>(m_device);
            m_shaderReload.m_graphicsReplacements[index].m_replacement = replacement;
            Jobs::DispatchBackground(
                [replacement, context = &graphicsContexts[index]] {
                    replacement->InitInternal(*context);
                },
                completion.Get());
        }

        for (uint32_t index = 0; index < computeContexts.size(); ++index)
        {
            Rc replacement = Memory::DefaultNew<ComputePipeline>(m_device);
            m_shaderReload.m_computeReplacements[index].m_replacement = replacement;
            Jobs::DispatchBackground(
                [replacement, context = &computeContexts[index]] {
                    replacement->InitInternal(*context);
                },
                completion.Get());
        }

        completion->Wait();
    }


    bool PipelineFactory::UsesAnyShader(const festd::span<const Env::Name> shaderNames, const Env::Name defines,
                                        const festd::span<const Core::ShaderHandle> shaders) const
    {
        for (const Env::Name shaderName : shaderNames)
        {
            if (!shaderName.IsValid())
                continue;

            const Core::ShaderHandle handle = m_shaderLibrary->FindShader(shaderName, defines);
            if (festd::find_index(shaders, handle) != kInvalidIndex)
                return true;
        }

        return false;
    }


    void PipelineFactory::ApplyShaderReload()
    {
        FE_PROFILER_ZONE();

        uint32_t swappedCount = 0;
        uint32_t failedCount = 0;
        const auto swap = [&swappedCount, &failedCount]<class TPipeline>(PipelineReplacement<TPipeline>& replacement) {
            if (!replacement.m_replacement->IsReady())
            {
                ++failedCount;
                return;
            }

            // The replacement then owns the previous native objects and disposes of them after the GPU is done.
            replacement.m_pipeline->SwapNative(*replacement.m_replacement);
            ++swappedCount;
        };

        for (PipelineReplacement<GraphicsPipeline>& replacement : m_shaderReload.m_graphicsReplacements)
            swap(replacement);

        for (PipelineReplacement<ComputePipeline>& replacement : m_shaderReload.m_computeReplacements)
            swap(replacement);

        const ShaderReload& reload = m_shaderReload;
        if (reload.m_recompiledShaderCount > 0 || reload.m_failedShaderCount > 0)
        {
            Logger::LogInfo("Shader hot reload: {} shaders recompiled, {} failed; {} pipelines replaced, {} failed",
                            reload.m_recompiledShaderCount,
                            reload.m_failedShaderCount,
                            swappedCount,
                            failedCount);
        }

        if (reload.m_recompiledShaderCount > 0)
            m_shaderReloadGeneration.fetch_add(1, std::memory_order_acq_rel);

        m_shaderReload = {};
    }
} // namespace FE::Graphics::Vulkan
