#pragma once
#include <FeCore/Memory/Object.h>
#include <OsGPU/Common/Viewport.h>
#include <OsGPU/Pipeline/InputStreamLayout.h>
#include <OsGPU/Pipeline/PipelineStates.h>

namespace FE::Osmium
{
    class IDescriptorTable;
    class IShaderModule;
    class IRenderPass;

    struct GraphicsPipelineDesc
    {
        Shared<IRenderPass> RenderPass;
        UInt32 SubpassIndex;

        List<Shared<IDescriptorTable>> DescriptorTables;
        List<Shared<IShaderModule>> Shaders;

        MultisampleState Multisample;
        RasterizationState Rasterization;
        DepthStencilState DepthStencil;
        ColorBlendState ColorBlend;
        InputStreamLayout InputLayout;
        Viewport Viewport;
        Scissor Scissor;

        FE_STRUCT_RTTI(GraphicsPipelineDesc, "1DA611B0-7064-4E66-B292-355ADB48548D");
    };

    class IGraphicsPipeline : public IObject
    {
    public:
        FE_CLASS_RTTI(IGraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~IGraphicsPipeline() override = default;
    };
} // namespace FE::Osmium
