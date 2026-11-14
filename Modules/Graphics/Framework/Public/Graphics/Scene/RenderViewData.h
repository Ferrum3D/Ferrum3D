#pragma once
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <Shaders/Lighting/ViewLighting.h>

namespace FE::Graphics
{
    //! Frame-local rendering resources available to optional view feature passes.
    struct RendererViewData final
    {
        Scene* m_scene = nullptr;
        View* m_view = nullptr;
        Core::Texture* m_mainColorTarget = nullptr;
        Core::Texture* m_mainDepthTarget = nullptr;
        //! Scene color presented by output passes: the main color target unless a module upscales it.
        Core::Texture* m_outputColorTarget = nullptr;
        //! Covers the main targets, which are smaller than m_outputResolution when a module upscales them.
        RectF m_viewportRect{ kForceInit };
        Vector2UInt m_outputResolution = Vector2UInt::kZero;
        //! Requested by a view module through ViewRenderSetup.
        bool m_needsMotionVectors = false;
        Core::RingUploader* m_renderQueueUploader = nullptr;
        ConstantsPtr<ViewData> m_constants;
        //! Created after the shadow passes of the view.
        ConstantsPtr<ViewLighting> m_lighting;
        bool m_isMainColorWritten = false;
    };
} // namespace FE::Graphics
