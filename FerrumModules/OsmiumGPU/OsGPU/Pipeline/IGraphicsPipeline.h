#pragma once
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
        IRenderPass* RenderPass;
        uint32_t SubpassIndex;

        ArraySlice<IDescriptorTable*> DescriptorTables;
        ArraySlice<IShaderModule*> Shaders;

        MultisampleState Multisample;
        RasterizationState Rasterization;
        DepthStencilState DepthStencil;
        ColorBlendState ColorBlend;
        InputStreamLayout InputLayout;
        Viewport Viewport;
        Scissor Scissor;

        FE_RTTI_Base(GraphicsPipelineDesc, "1DA611B0-7064-4E66-B292-355ADB48548D");
    };

    class IGraphicsPipeline : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IGraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~IGraphicsPipeline() override = default;
    };
} // namespace FE::Osmium
