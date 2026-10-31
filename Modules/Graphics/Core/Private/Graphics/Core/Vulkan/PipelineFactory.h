#pragma once
#include <Core/IO/Path.h>
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ShaderCompiler.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    struct ShaderCompilerDXC;
}

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary;
    struct DescriptorManager;

    struct PipelineFactory final : Core::PipelineFactory
    {
        FE_RTTI("437E4387-BDE0-42DA-8986-FA909D8BFEDE");

        PipelineFactory(Core::Device* device, Core::DescriptorManager* descriptorManager);
        ~PipelineFactory() override;

        Core::GraphicsPipeline* CreateGraphicsPipeline(const Core::GraphicsPipelineRequest& request) override;
        Core::ComputePipeline* CreateComputePipeline(const Core::ComputePipelineRequest& request) override;
        Core::ShaderLibrary* GetShaderLibrary() const override;

        [[nodiscard]] uint32_t GetShaderReloadGeneration() const override
        {
            return m_shaderReloadGeneration.load(std::memory_order_acquire);
        }

        //! @brief Advance shader hot reload; does nothing outside development builds.
        //!
        //! Polls the shader source directories, starts a background reload once changes have settled and swaps the rebuilt
        //! pipelines in place when it completes. Must be called while no commands are being recorded; the device does so at
        //! the end of every frame.
        void UpdateShaderHotReload();

    private:
        template<class TPipeline>
        struct PipelineEntry final
        {
            TPipeline* m_pipeline = nullptr;
            typename TPipeline::InitContext m_context;
        };


        template<class TPipeline>
        struct PipelineReplacement final
        {
            TPipeline* m_pipeline = nullptr;
            Rc<TPipeline> m_replacement;
        };


        //! State shared with the background reload job; only touched by UpdateShaderHotReload once the job has completed.
        struct ShaderReload final
        {
            festd::vector<Env::Name> m_changedPaths;
            uint32_t m_recompiledShaderCount = 0;
            uint32_t m_failedShaderCount = 0;
            festd::vector<PipelineReplacement<GraphicsPipeline>> m_graphicsReplacements;
            festd::vector<PipelineReplacement<ComputePipeline>> m_computeReplacements;
        };

        void PrepareShaderReload();
        void RecompileShaders(festd::span<const Core::ShaderHandle> shaders, festd::vector<Core::ShaderHandle>& recompiled);
        void BuildReplacementPipelines(festd::span<const Core::ShaderHandle> recompiledShaders);
        [[nodiscard]] bool UsesAnyShader(festd::span<const Env::Name> shaderNames, Env::Name defines,
                                         festd::span<const Core::ShaderHandle> shaders) const;
        void ApplyShaderReload();

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        FE_PROFILER_LOCK(Threading::SpinLock, m_lock);
        Rc<Core::ShaderCompilerDXC> m_shaderCompiler;
        Rc<ShaderLibrary> m_shaderLibrary;
        DescriptorManager* m_descriptorManager = nullptr;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        IO::Path m_pipelineCachePath;
        festd::unordered_dense_map<uint64_t, PipelineEntry<GraphicsPipeline>> m_graphicsPipelinesMap;
        festd::unordered_dense_map<uint64_t, PipelineEntry<ComputePipeline>> m_computePipelinesMap;

        std::atomic<uint32_t> m_shaderReloadGeneration = 0;
        festd::vector<Env::Name> m_pendingChangedPaths;
        uint64_t m_lastChangeTicks = 0;
        Rc<WaitGroup> m_shaderReloadCompletion;
        ShaderReload m_shaderReload;
    };
} // namespace FE::Graphics::Vulkan
