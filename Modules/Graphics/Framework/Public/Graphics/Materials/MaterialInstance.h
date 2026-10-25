#pragma once
#include <Graphics/Assets/MaterialAssets.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ShaderReflection.h>
#include <Graphics/Materials/ParameterAllocator.h>

namespace FE::Graphics
{
    struct MaterialInstanceRuntime final
    {
        MaterialInstanceRuntime(const MaterialAsset* material, const MaterialInstanceAsset* instance,
                                MaterialParameterAllocator* allocator, Core::PipelineFactory* pipelineFactory);
        ~MaterialInstanceRuntime();

        MaterialInstanceRuntime(const MaterialInstanceRuntime&) = delete;
        MaterialInstanceRuntime& operator=(const MaterialInstanceRuntime&) = delete;

        [[nodiscard]] Core::GraphicsPipeline* GetPipeline(Env::Name role);
        [[nodiscard]] Core::ComputePipeline* GetComputePipeline(Env::Name role, uint32_t waveLaneCount = 32);
        [[nodiscard]] bool HasTechnique(Env::Name role) const;
        [[nodiscard]] BufferPointer GetMaterialParameters() const;
        [[nodiscard]] MaterialParameterAllocator::Allocation AllocateInstanceParameters();
        void FreeInstanceParameters(MaterialParameterAllocator::Allocation allocation);
        void RefreshDescriptors();

    private:
        struct PipelineEntry final
        {
            Env::Name m_role;
            Core::GraphicsPipeline* m_pipeline = nullptr;
            Core::ComputePipeline* m_computePipeline = nullptr;
            uint32_t m_waveLaneCount = 32;
        };

        [[nodiscard]] bool MergeTechniqueLayouts(Core::ShaderStructLayout& materialLayout,
                                                 Core::ShaderStructLayout& instanceLayout) const;
        [[nodiscard]] bool MergeShaderLayouts(Env::Name shader, Env::Name defines, Core::ShaderStructLayout& materialLayout,
                                              Core::ShaderStructLayout& instanceLayout) const;

        //! Re-merge parameter layouts after shader hot reload, keeping the current ones if the new ones cannot be applied.
        void RefreshLayouts();
        [[nodiscard]] bool HasAllParameters(const Core::ShaderStructLayout& layout, MaterialParameterScope scope) const;
        const MaterialTechniqueDesc* FindTechnique(Env::Name role) const;
        const MaterialParameterValue* FindValue(Env::Name name) const;
        festd::vector<std::byte> PackParameters(MaterialParameterScope scope) const;
        const Core::ShaderStructLayout& GetLayout(MaterialParameterScope scope) const;

        const MaterialAsset* m_material = nullptr;
        const MaterialInstanceAsset* m_instance = nullptr;
        MaterialParameterAllocator* m_allocator = nullptr;
        Core::PipelineFactory* m_pipelineFactory = nullptr;
        Core::ShaderStructLayout m_materialLayout;
        Core::ShaderStructLayout m_instanceLayout;
        uint32_t m_shaderReloadGeneration = 0;
        MaterialParameterAllocator::Allocation m_materialAllocation;
        festd::vector<MaterialParameterAllocator::Allocation> m_instanceAllocations;
        festd::vector<std::byte> m_materialBytes;
        festd::vector<std::byte> m_instanceBytes;
        festd::vector<PipelineEntry> m_pipelines;
    };
} // namespace FE::Graphics
