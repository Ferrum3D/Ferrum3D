#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::ScreenSpaceShadow
{
    //! Published by AddPasses for views whose sun is shadowed in screen space.
    struct Output final
    {
        //! Fraction of the sun that the camera depth leaves visible at each pixel of the main targets. It finds occluders
        //! within kScreenShadowSampleCount pixels of the surface, so it complements the shadow maps and does not replace
        //! them.
        Rc<Core::Texture> m_sunVisibility;
    };


    struct LightDesc final
    {
        //! The light projected by the camera that rendered the depth, before the division by w: a position with w = 1 or
        //! the direction towards a directional light with w = 0.
        Vector4 m_clipPosition = Vector4::kZero;
        //! Thickness assumed for every pixel of the depth, as a fraction of its depth.
        float m_surfaceThickness = 0.005f;
        //! Sharpens the transition in and out of shadow; at least one.
        float m_shadowContrast = 4.0f;
    };


    //! Write the fraction of the light that a reverse Z depth leaves visible at each of its pixels into a new texture of
    //! its size. Returns null when the light needs no dispatch.
    [[nodiscard]] Rc<Core::Texture> AddLightPass(Core::FrameGraph& graph, Core::Texture* depth, const LightDesc& light);

    //! Shadow the sun of a deferred view by marching its main depth towards the light with Bend Studio's screen space
    //! shadows; requires the sun shadows of the view, whose settings configure the pass.
    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::ScreenSpaceShadow
