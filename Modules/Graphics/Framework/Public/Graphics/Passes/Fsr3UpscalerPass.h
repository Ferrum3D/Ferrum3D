#pragma once
#include <Graphics/Scene/View.h>

namespace FE::Graphics::Fsr3
{
    struct History;


    //! Published by AddPasses; RendererViewData::m_outputColorTarget refers to m_color.
    struct Output final
    {
        Rc<Core::Texture> m_color;
    };


    //! Optional masks supplied by effects whose shading cannot be described by motion vectors.
    struct Masks final
    {
        Rc<Core::Texture> m_transparencyAndComposition;
    };


    struct ViewModule final : ViewModuleBase
    {
        FE_RTTI("B0D3303E-1C18-40D0-8D96-650924AC80B7");

        explicit ViewModule(View* view);
        ~ViewModule() override;

        //! Internal resolution relative to the output: 2/3 is FSR Quality; 1 is Native AA.
        float m_renderScale = 2.0f / 3.0f;

        //! Call for camera cuts or discontinuous scene changes.
        void ResetHistory();

        //! Reduces the render resolution, jitters the projection and requests motion vectors.
        void PrepareView(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, ViewRenderSetup& setup) override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;

        festd::unique_ptr<History> m_history;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::Fsr3
