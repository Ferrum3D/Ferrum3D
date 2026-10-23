#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Scene/Culling.h>

namespace FE::Graphics::Culling
{
    namespace
    {
        float GetProjectionScaleY(const View& view)
        {
            return view.GetProjectionMatrix().m_11;
        }
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        if (m_settings.m_freezeCulling != m_isFrozen)
        {
            // A new freeze captures a new depth pyramid; unfreezing releases it.
            m_frozenDepthPyramid = {};
            m_frozenViewProjection = m_view->GetViewProjectionMatrix();
            m_frozenCameraPosition = m_view->GetCameraTransform().Translation();
            m_frozenProjectionScaleY = GetProjectionScaleY(*m_view);
        }

        m_isFrozen = m_settings.m_freezeCulling;

        PassData& data = blackboard.Add<PassData>();
        data.m_settings = m_settings;
        data.m_stats = &m_stats;
        if (m_isFrozen)
        {
            data.m_viewProjection = m_frozenViewProjection;
            data.m_occlusionViewProjection = m_frozenViewProjection;
            data.m_cameraPosition = m_frozenCameraPosition;
            data.m_projectionScaleY = m_frozenProjectionScaleY;
            data.m_frozenDepthPyramid = &m_frozenDepthPyramid;
        }
        else
        {
            data.m_viewProjection = m_view->GetViewProjectionMatrix();
            data.m_occlusionViewProjection = data.m_viewProjection;
            data.m_cameraPosition = m_view->GetCameraTransform().Translation();
            data.m_projectionScaleY = GetProjectionScaleY(*m_view);
        }
    }


    PassData& GetPassData(Core::FrameGraphBlackboard& blackboard)
    {
        if (PassData* data = blackboard.TryGet<PassData>())
            return *data;

        const View& view = *blackboard.Get<RendererViewData>().m_view;
        PassData& data = blackboard.Add<PassData>();
        data.m_viewProjection = view.GetViewProjectionMatrix();
        data.m_occlusionViewProjection = data.m_viewProjection;
        data.m_cameraPosition = view.GetCameraTransform().Translation();
        data.m_projectionScaleY = GetProjectionScaleY(view);
        return data;
    }
} // namespace FE::Graphics::Culling
