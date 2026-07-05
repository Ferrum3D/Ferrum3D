#pragma once
#include <Core/Jobs/IJobSystem.h>
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary;
    struct GraphicsPipeline;
    struct ComputePipeline;
    struct DescriptorManager;

    struct PipelineFactory final : Core::PipelineFactory
    {
        PipelineFactory(Core::Device* device, Core::DescriptorManager* descriptorManager, IJobSystem* jobSystem);
        ~PipelineFactory() override;

        FE_RTTI("437E4387-BDE0-42DA-8986-FA909D8BFEDE");

        Core::GraphicsPipeline* CreateGraphicsPipeline(const Core::GraphicsPipelineRequest& request) override;
        Core::ComputePipeline* CreateComputePipeline(const Core::ComputePipelineRequest& request) override;

    private:
        template<class TPipeline>
        struct AsyncCompilationJob;

        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }

        FE_PROFILER_LOCK(Threading::SpinLock, m_lock);
        Rc<ShaderLibrary> m_shaderLibrary;
        Memory::SpinLockedPoolAllocator m_jobPool;
        DescriptorManager* m_descriptorManager = nullptr;
        IJobSystem* m_jobSystem = nullptr;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        festd::unordered_dense_map<uint64_t, GraphicsPipeline*> m_graphicsPipelinesMap;
        festd::unordered_dense_map<uint64_t, ComputePipeline*> m_computePipelinesMap;
    };
} // namespace FE::Graphics::Vulkan
