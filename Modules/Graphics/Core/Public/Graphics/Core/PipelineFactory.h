#pragma once
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/ShaderLibrary.h>

namespace FE::Graphics::Core
{
    enum class PipelinePriority : uint32_t
    {
        kNormal,
        kHigh,
        kHighest,
        kCount,
    };


    struct GraphicsPipelineRequest final
    {
        PipelinePriority m_priority = PipelinePriority::kNormal;
        festd::span<const ShaderDefine> m_defines;
        GraphicsPipelineDesc m_desc;

        [[nodiscard]] uint64_t GetHash() const
        {
            FE_PROFILER_ZONE();

            Hasher hasher;
            hasher.UpdateRaw(m_desc.GetHash());
            for (const ShaderDefine& define : m_defines)
                hasher.UpdateRaw(define.GetHash());

            return hasher.Finalize();
        }
    };


    struct PipelineFactory : public DeviceObject
    {
        ~PipelineFactory() override = default;

        FE_RTTI_Class(PipelineFactory, "CD16508A-5F34-4700-8D23-AF217B55FFD1");

        virtual GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineRequest& request) = 0;
    };
} // namespace FE::Graphics::Core
