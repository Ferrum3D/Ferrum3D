#pragma once
#include <FeCore/Memory/Object.h>
#include <GPU/Common/Viewport.h>
#include <GPU/Descriptors/IDescriptorTable.h>
#include <GPU/Pipeline/InputStreamLayout.h>
#include <GPU/Pipeline/PipelineStates.h>
#include <GPU/RenderPass/IRenderPass.h>
#include <GPU/Shader/IShaderModule.h>

namespace FE::GPU
{
    struct GraphicsPipelineDesc
    {
        Shared<IRenderPass> RenderPass;
        UInt32 SubpassIndex;

        Vector<Shared<IDescriptorTable>> DescriptorTables;
        Vector<Shared<IShaderModule>> Shaders;

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
} // namespace FE::GPU
