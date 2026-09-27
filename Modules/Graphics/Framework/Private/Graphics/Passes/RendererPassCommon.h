#pragma once
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics
{
    struct RendererViewData final
    {
        Scene* m_scene = nullptr;
        View* m_view = nullptr;
        Core::Viewport* m_viewport = nullptr;
        Core::Texture* m_mainColorTarget = nullptr;
        Core::Texture* m_mainDepthTarget = nullptr;
        RectF m_viewportRect{ kForceInit };
        DB::Database* m_database = nullptr;
        Core::RingUploader* m_renderQueueUploader = nullptr;
    };


    struct RendererTargetClearPassDesc final
    {
        Core::PassColorTarget m_colorTarget;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("E92985A4-5D67-4CAA-82E7-A4D702E97C27");
    };
} // namespace FE::Graphics
