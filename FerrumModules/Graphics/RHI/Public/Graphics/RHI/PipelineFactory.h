#pragma once
#include <Graphics/RHI/GraphicsPipeline.h>

namespace FE::Graphics::RHI
{
    struct PipelineFactory : public DeviceObject
    {
        ~PipelineFactory() override = default;

        FE_RTTI_Class(PipelineFactory, "CD16508A-5F34-4700-8D23-AF217B55FFD1");

        virtual festd::expected<GraphicsPipeline*, ResultCode> CreateGraphicsPipeline(Env::Name name,
                                                                                      const GraphicsPipelineDesc& desc) = 0;
    };
} // namespace FE::Graphics::RHI
