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
        };

        const MaterialParameterValue* FindValue(Env::Name name) const;
        festd::vector<std::byte> PackParameters(MaterialParameterScope scope) const;
        const Core::ShaderStructLayout& GetLayout(MaterialParameterScope scope) const;

        const MaterialAsset* m_material = nullptr;
        const MaterialInstanceAsset* m_instance = nullptr;
        MaterialParameterAllocator* m_allocator = nullptr;
        Core::PipelineFactory* m_pipelineFactory = nullptr;
        Core::ShaderStructLayout m_materialLayout;
        Core::ShaderStructLayout m_instanceLayout;
        MaterialParameterAllocator::Allocation m_materialAllocation;
        festd::vector<MaterialParameterAllocator::Allocation> m_instanceAllocations;
        festd::vector<std::byte> m_materialBytes;
        festd::vector<std::byte> m_instanceBytes;
        festd::vector<PipelineEntry> m_pipelines;
    };
} // namespace FE::Graphics
