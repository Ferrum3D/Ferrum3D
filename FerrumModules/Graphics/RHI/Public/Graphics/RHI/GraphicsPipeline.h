#pragma once
#include <Graphics/RHI/Common/Viewport.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/InputStreamLayout.h>
#include <Graphics/RHI/PipelineStates.h>
#include <Graphics/RHI/ShaderStage.h>

namespace FE::Graphics::RHI
{
    struct ShaderResourceGroup;
    struct ShaderModule;
    struct RenderPass;


    struct GraphicsPipelineDesc final
    {
        ShaderResourceGroup* m_shaderResourceGroups[Limits::Pipeline::kMaxShaderResourceGroups];
        ShaderModule* m_shaders[festd::to_underlying(ShaderStage::kGraphicsCount)];

        RenderPass* m_renderPass = nullptr;
        uint32_t m_subpassIndex = 0;

        int32_t m_sampleCount = 1;
        RasterizationState m_rasterization;
        DepthStencilState m_depthStencil;
        ColorBlendState m_colorBlend;
        InputStreamLayout m_inputLayout;
        Viewport m_viewport = Viewport::kInvalid;
        Scissor m_scissor = Scissor::kInvalid;

        GraphicsPipelineDesc& SetShaders(ShaderModule* vertex, ShaderModule* pixel)
        {
            m_shaders[festd::to_underlying(ShaderStage::kVertex)] = vertex;
            m_shaders[festd::to_underlying(ShaderStage::kPixel)] = pixel;
            return *this;
        }

        GraphicsPipelineDesc& SetRasterization(const RasterizationState rasterization)
        {
            m_rasterization = rasterization;
            return *this;
        }

        GraphicsPipelineDesc& SetDepthStencil(const DepthStencilState depthStencil)
        {
            m_depthStencil = depthStencil;
            return *this;
        }

        GraphicsPipelineDesc& SetColorBlend(const ColorBlendState& colorBlend)
        {
            m_colorBlend = colorBlend;
            return *this;
        }

        GraphicsPipelineDesc& SetColorBlend(const TargetColorBlending colorBlend)
        {
            m_colorBlend.m_targetBlendStates[0] = colorBlend;
            m_colorBlend.m_enableIndependentBlend = false;
            return *this;
        }

        GraphicsPipelineDesc& SetInputLayout(const InputStreamLayout& inputLayout)
        {
            m_inputLayout = inputLayout;
            return *this;
        }

        GraphicsPipelineDesc& SetViewportAndScissor(const Viewport& viewport)
        {
            m_viewport = viewport;
            m_scissor = Scissor(viewport);
            return *this;
        }
    };


    struct GraphicsPipeline : public DeviceObject
    {
        FE_RTTI_Class(GraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~GraphicsPipeline() override = default;

        [[nodiscard]] Env::Name GetName() const
        {
            return m_name;
        }

    protected:
        Env::Name m_name;
    };
} // namespace FE::Graphics::RHI
