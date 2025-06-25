#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary;
    struct GraphicsPipeline;
    struct ComputePipeline;
    struct BindlessManager;

    struct PipelineFactory final : Core::PipelineFactory
    {
        PipelineFactory(Core::Device* device, BindlessManager* bindlessManager, IJobSystem* jobSystem, Logger* logger);
        ~PipelineFactory() override;

        FE_RTTI_Class(PipelineFactory, "437E4387-BDE0-42DA-8986-FA909D8BFEDE");

        Core::GraphicsPipeline* CreateGraphicsPipeline(const Core::GraphicsPipelineRequest& request) override;
        Core::ComputePipeline* CreateComputePipeline(const Core::ComputePipelineRequest& request) override;

    private:
        template<class TPipeline>
        struct AsyncCompilationJob;

        FE_PROFILER_LOCK(Threading::SpinLock, m_lock);
        Rc<ShaderLibrary> m_shaderLibrary;
        Memory::PoolAllocator m_graphicsPipelinePool;
        Memory::PoolAllocator m_computePipelinePool;
        Memory::SpinLockedPoolAllocator m_jobPool;
        BindlessManager* m_bindlessManager = nullptr;
        IJobSystem* m_jobSystem = nullptr;
        Logger* m_logger = nullptr;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        festd::unordered_dense_map<uint64_t, GraphicsPipeline*> m_graphicsPipelinesMap;
        festd::unordered_dense_map<uint64_t, ComputePipeline*> m_computePipelinesMap;
    };
} // namespace FE::Graphics::Vulkan
