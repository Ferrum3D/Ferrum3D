#pragma once
#include <Graphics/RHI/Common/Viewport.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/InputStreamLayout.h>
#include <Graphics/RHI/PipelineStates.h>

namespace FE::Graphics::RHI
{
    struct ShaderResourceGroup;
    struct ShaderModule;
    struct RenderPass;


    struct GraphicsPipelineDesc final
    {
        RenderPass* m_renderPass = nullptr;
        uint32_t m_subpassIndex = 0;

        festd::span<ShaderResourceGroup* const> m_shaderResourceGroups;
        festd::span<ShaderModule* const> m_shaders;

        MultisampleState m_multisample;
        RasterizationState m_rasterization;
        DepthStencilState m_depthStencil;
        ColorBlendState m_colorBlend;
        InputStreamLayout m_inputLayout;
        Viewport m_viewport;
        Scissor m_scissor;
    };


    struct GraphicsPipeline : public DeviceObject
    {
        FE_RTTI_Class(GraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~GraphicsPipeline() override = default;

        virtual ResultCode Init(const GraphicsPipelineDesc& desc) = 0;
    };
} // namespace FE::Graphics::RHI
