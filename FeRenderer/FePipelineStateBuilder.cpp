#include "FePipelineStateBuilder.h"
#include <FeCore/Console/FeLog.h>
#include <FeRenderer/FeShader.h>
#include <FeRenderer/FeVertexLayout.h>

namespace FE
{
    namespace DL = Diligent;

    FePipelineStateBuilder::FePipelineStateBuilder(const std::string& name, DL::ISwapChain* swapChain)
    {
        m_Name                        = name;
        m_GraphicsPSInfo.PSODesc.Name = m_Name.c_str();

        m_GraphicsPSInfo.GraphicsPipeline.NumRenderTargets             = 1;
        m_GraphicsPSInfo.GraphicsPipeline.RTVFormats[0]                = swapChain->GetDesc().ColorBufferFormat;
        m_GraphicsPSInfo.GraphicsPipeline.DSVFormat                    = swapChain->GetDesc().DepthBufferFormat;
        m_GraphicsPSInfo.GraphicsPipeline.PrimitiveTopology            = DL::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_GraphicsPSInfo.GraphicsPipeline.RasterizerDesc.CullMode      = DL::CULL_MODE_BACK;
        m_GraphicsPSInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;

        m_GraphicsPSInfo.PSODesc.ResourceLayout.DefaultVariableType = DL::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

        // TODO: Variables, immutable samplers
        // ShaderResourceVariableDesc Vars[] =
        // {
        //     {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
        // };
        // m_GraphicsPSInfo.PSODesc.ResourceLayout.Variables = Vars;
        // m_GraphicsPSInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);
        //  ...
        // m_GraphicsPSInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
        // m_GraphicsPSInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    }

    void FePipelineStateBuilder::SetPipelineType(FePipelineType type)
    {
        m_Type = type;
        switch (type)
        {
        case FE::FePipelineType::Graphics:
            m_GraphicsPSInfo.PSODesc.PipelineType = DL::PIPELINE_TYPE_GRAPHICS;
            break;
        case FE::FePipelineType::Compute:
            FE_ASSERT_MSG(false, "Compute pipelines aren't implemented");
            break;
        }
    }

    void FePipelineStateBuilder::SetShaders(const FeShaderProgramDesc& desc)
    {
        FE_ASSERT_MSG(m_Type != FePipelineType::None, "Pipeline type must be set before other calls");

        auto* ps = (FeShader*)desc.PixelShader.get();
        auto* vs = (FeShader*)desc.VertexShader.get();

        if (ps)
            m_GraphicsPSInfo.pPS = ps->GetHandle();
        if (vs)
            m_GraphicsPSInfo.pVS = vs->GetHandle();
    }

    std::shared_ptr<IFeVertexLayout> FePipelineStateBuilder::GetVertexLayout(FeVertexLayoutType type)
    {
        auto layout = m_VertexLayouts[size_t(type)];
        FE_ASSERT_MSG(layout, "Layout must be set before call to GetVertexLayout");
        return layout;
    }

    void FePipelineStateBuilder::SetVertexLayout(IFeVertexLayout* layout)
    {
        auto* l = (FeVertexLayout*)layout;

        m_GraphicsPSInfo.GraphicsPipeline.InputLayout.LayoutElements = l->GetElements();
        m_GraphicsPSInfo.GraphicsPipeline.InputLayout.NumElements    = l->GetElementCount();
    }

    void FePipelineStateBuilder::RegisterVertexLayout(FeVertexLayoutType type, std::shared_ptr<IFeVertexLayout> layout)
    {
        m_VertexLayouts[size_t(type)] = std::move(layout);
    }

    FePipelineState FePipelineStateBuilder::Build(DL::IRenderDevice* device)
    {
        FePipelineState state{};
        device->CreateGraphicsPipelineState(m_GraphicsPSInfo, &state.PSO);
        // TODO: variables
        // state.PSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);
        state.PSO->CreateShaderResourceBinding(&state.SRB);
        return state;
    }
} // namespace FE
