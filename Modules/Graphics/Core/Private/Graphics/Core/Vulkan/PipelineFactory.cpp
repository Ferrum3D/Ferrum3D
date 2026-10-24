#include <Core/IO/FileStream.h>
#include <Core/Jobs/Jobs.h>
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
            return it->second;

        auto* pipeline = Memory::DefaultNew<GraphicsPipeline>(m_device);
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

        auto* pipeline = Memory::DefaultNew<ComputePipeline>(m_device);
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
