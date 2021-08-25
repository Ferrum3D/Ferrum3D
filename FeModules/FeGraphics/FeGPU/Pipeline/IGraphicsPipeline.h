#pragma once
#include <FeGPU/Pipeline/PipelineStates.h>
#include <FeCore/Memory/Object.h>
#include <FeGPU/Descriptors/IDescriptorTable.h>
#include <FeGPU/Shader/IShaderModule.h>
#include <FeGPU/Pipeline/InputStreamLayout.h>
#include <FeGPU/RenderPass/IRenderPass.h>

namespace FE::GPU
{
    struct Viewport
    {
        Float32 MinX;
        Float32 MinY;
        Float32 MinZ;
        Float32 MaxX;
        Float32 MaxY;
        Float32 MaxZ;

        FE_STRUCT_RTTI(Viewport, "0BD79E66-6539-4AD3-A6DC-66066B56C5BF");

        [[nodiscard]] inline Float32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        [[nodiscard]] inline Float32 Height() const noexcept
        {
            return MaxY - MinY;
        }

        [[nodiscard]] inline Float32 Depth() const noexcept
        {
            return MaxZ - MinZ;
        }
    };

    struct Scissor
    {
        Int32 MinX;
        Int32 MinY;
        Int32 MaxX;
        Int32 MaxY;

        FE_STRUCT_RTTI(Scissor, "BC7244C7-821B-4044-B408-219A2BE1A955");

        [[nodiscard]] inline Int32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        [[nodiscard]] inline Int32 Height() const noexcept
        {
            return MaxY - MinY;
        }
    };

    struct GraphicsPipelineDesc
    {
        FE_STRUCT_RTTI(GraphicsPipelineDesc, "1DA611B0-7064-4E66-B292-355ADB48548D");

        RefCountPtr<IRenderPass> RenderPass;

        Vector<RefCountPtr<IDescriptorTable>> DescriptorTables;
        Vector<RefCountPtr<IShaderModule>> Shaders;

        RasterizationState Rasterization;
        DepthStencilState DepthStencil;
        ColorBlendState ColorBlend;
        InputStreamLayout InputLayout;
        Viewport Viewport;
        Scissor Scissor;
    };

    class IGraphicsPipeline : public IObject
    {
    public:
        FE_CLASS_RTTI(IGraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~IGraphicsPipeline() override = default;
    };
}
