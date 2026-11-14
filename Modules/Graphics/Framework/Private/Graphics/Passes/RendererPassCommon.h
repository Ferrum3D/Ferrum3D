#pragma once
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Scene/RenderViewData.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/View.h>
#include <Shaders/Scene/ViewData.h>

namespace FE::Graphics
{
    void ClearMainTargets(Core::FrameGraph& graph, const RendererViewData& view);


    //! @brief The matrix scene geometry is rasterized with, including the sub-pixel jitter of a temporal module.
    [[nodiscard]] inline Matrix4x4 GetRasterViewProjection(Core::FrameGraphBlackboard& blackboard, const View& view)
    {
        if (const ViewData* camera = blackboard.TryGet<ViewData>())
            return camera->m_viewProjection;

        return view.GetViewProjectionMatrix();
    }


    struct RendererTargetClearPassDesc final
    {
        Core::PassColorTarget m_colorTarget;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("E92985A4-5D67-4CAA-82E7-A4D702E97C27");
    };
} // namespace FE::Graphics
