#pragma once
#include <Graphics/RHI/GraphicsPipeline.h>

namespace FE::Graphics::RHI
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
        Env::Name m_pipelineName;
        PipelinePriority m_priority = PipelinePriority::kNormal;
        GraphicsPipelineDesc m_desc;
    };


    struct PipelineFactory : public DeviceObject
    {
        ~PipelineFactory() override = default;

        FE_RTTI_Class(PipelineFactory, "CD16508A-5F34-4700-8D23-AF217B55FFD1");

        virtual GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineRequest& request) = 0;

        virtual void DispatchPending() = 0;
    };
} // namespace FE::Graphics::RHI
