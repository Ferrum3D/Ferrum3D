#pragma once
#include <HAL/Common/Viewport.h>
#include <HAL/DeviceObject.h>
#include <HAL/InputStreamLayout.h>
#include <HAL/PipelineStates.h>

namespace FE::Graphics::HAL
{
    class ShaderResourceGroup;
    class ShaderModule;
    class RenderPass;


    struct GraphicsPipelineDesc final
    {
        RenderPass* RenderPass = nullptr;
        uint32_t SubpassIndex = 0;

        festd::span<ShaderResourceGroup*> ShaderResourceGroups;
        festd::span<ShaderModule*> Shaders;

        MultisampleState Multisample;
        RasterizationState Rasterization;
        DepthStencilState DepthStencil;
        ColorBlendState ColorBlend;
        InputStreamLayout InputLayout;
        Viewport Viewport;
        Scissor Scissor;
    };


    class GraphicsPipeline : public DeviceObject
    {
    public:
        FE_RTTI_Class(GraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~GraphicsPipeline() override = default;

        virtual ResultCode Init(const GraphicsPipelineDesc& desc) = 0;
    };
} // namespace FE::Graphics::HAL
