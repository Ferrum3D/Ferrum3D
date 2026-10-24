#pragma once
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics
{
    //! Frame-local rendering resources available to optional view feature passes.
    struct RendererViewData final
    {
        Scene* m_scene = nullptr;
        View* m_view = nullptr;
        Core::Texture* m_mainColorTarget = nullptr;
        Core::Texture* m_mainDepthTarget = nullptr;
        RectF m_viewportRect{ kForceInit };
        Core::RingUploader* m_renderQueueUploader = nullptr;
        Rc<Core::Buffer> m_constants;
        bool m_isMainColorWritten = false;
    };
} // namespace FE::Graphics
