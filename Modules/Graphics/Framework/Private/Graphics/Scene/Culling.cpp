#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Scene/Culling.h>

namespace FE::Graphics::Culling
{
    namespace
    {
        uint32_t CalculateCullFlags(const Culling::Settings& settings)
        {
            uint32_t flags = 0;
            if (settings.m_instanceFrustumCulling)
                flags |= MeshPass::CullFlags::kInstanceFrustum;
            if (settings.m_instanceOcclusionCulling)
                flags |= MeshPass::CullFlags::kInstanceOcclusion;
            if (settings.m_meshletFrustumCulling)
                flags |= MeshPass::CullFlags::kMeshletFrustum;
            if (settings.m_meshletOcclusionCulling)
                flags |= MeshPass::CullFlags::kMeshletOcclusion;
            if (settings.m_lodSelection)
                flags |= MeshPass::CullFlags::kLodSelection;

            return flags;
        }


        float GetProjectionScaleY(const View& view)
        {
            return view.GetProjectionMatrix().m_11;
        }
    } // namespace


    MeshPass::MeshCullData BuildMeshCullData(const PassData& culling, const float viewportHeight)
    {
        // An LOD is acceptable when error * scale / distance * projection * height / 2 <= threshold. Orthographic views
        // compare against a unit distance.
        const Culling::PassData& reference = culling.m_lodReference != nullptr ? *culling.m_lodReference : culling;
        const float height = culling.m_lodReference != nullptr ? culling.m_lodViewportHeight : viewportHeight;
        const float threshold = Math::Max(reference.m_settings.m_lodErrorThreshold, 1e-3f);
        const float lodScale = reference.m_projectionScaleY * height * 0.5f / threshold;

        MeshPass::MeshCullData cull{};
        cull.m_viewProjection = culling.m_viewProjection;
        cull.m_occlusionViewProjection = culling.m_occlusionViewProjection;
        cull.m_cameraPosition = PackedVector4F(Vector4(reference.m_cameraPosition, lodScale));
        cull.m_flags = CalculateCullFlags(culling.m_settings);
        cull.m_flags &= ~MeshPass::CullFlags::kLodSelection;
        if (reference.m_settings.m_lodSelection)
            cull.m_flags |= MeshPass::CullFlags::kLodSelection;
        if (reference.m_isOrthographic)
            cull.m_flags |= MeshPass::CullFlags::kOrthographic;

        return cull;
    }


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
            m_isFrozenOrthographic = m_view->IsOrthographic();
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
            data.m_isOrthographic = m_isFrozenOrthographic;
            data.m_frozenDepthPyramid = &m_frozenDepthPyramid;
        }
        else
        {
            data.m_viewProjection = m_view->GetViewProjectionMatrix();
            data.m_occlusionViewProjection = data.m_viewProjection;
            data.m_cameraPosition = m_view->GetCameraTransform().Translation();
            data.m_projectionScaleY = GetProjectionScaleY(*m_view);
            data.m_isOrthographic = m_view->IsOrthographic();
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
        data.m_isOrthographic = view.IsOrthographic();
        return data;
    }
} // namespace FE::Graphics::Culling
