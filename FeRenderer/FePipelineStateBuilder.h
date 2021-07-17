#pragma once
#include "FeGraphicsDevice.h"
#include "IFePipelineStateBuilder.h"

namespace FE
{
    struct FePipelineState
    {
        DL::RefCntAutoPtr<DL::IPipelineState> PSO;
        DL::RefCntAutoPtr<DL::IShaderResourceBinding> SRB;
    };

    class FePipelineStateBuilder : public IFePipelineStateBuilder
    {
        FePipelineType m_Type = FePipelineType::None;
        std::string m_Name;

        DL::GraphicsPipelineStateCreateInfo m_GraphicsPSInfo{};
        std::shared_ptr<IFeVertexLayout> m_VertexLayouts[size_t(FeVertexLayoutType::Count)];

    public:
        FePipelineStateBuilder(const std::string& name, DL::ISwapChain* swapChain);

        virtual void SetPipelineType(FePipelineType type) override;
        virtual void SetShaders(const FeShaderProgramDesc& desc) override;
        virtual std::shared_ptr<IFeVertexLayout> GetVertexLayout(FeVertexLayoutType type) override;
        virtual void SetVertexLayout(IFeVertexLayout* layout) override;

        void RegisterVertexLayout(FeVertexLayoutType type, std::shared_ptr<IFeVertexLayout> layout);
        FePipelineState Build(DL::IRenderDevice* device);
    };
} // namespace FE
