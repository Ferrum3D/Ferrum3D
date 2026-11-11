#pragma once
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Graphics/Scene/View.h>
#include <Shaders/Passes/MeshPass/GpuWork.h>

namespace FE::Graphics
{
    struct CulledMeshView;
}

namespace FE::Graphics::Culling
{
    struct Settings final
    {
        bool m_instanceFrustumCulling = true;
        bool m_instanceOcclusionCulling = true;
        bool m_meshletFrustumCulling = true;
        //! Meshlets hidden in the previous frame are skipped by the early phase and retested by the late phase.
        bool m_meshletOcclusionCulling = true;
        bool m_lodSelection = true;
        //! The largest projected simplification error, in pixels, that LOD selection accepts.
        float m_lodErrorThreshold = 1.0f;
        //! Keep all culling at the camera of the frame the freeze started. Occlusion culling then tests against the depth
        //! pyramid captured in that frame.
        bool m_freezeCulling = false;

        [[nodiscard]] bool IsOcclusionCullingEnabled() const
        {
            return m_instanceOcclusionCulling || m_meshletOcclusionCulling;
        }
    };


    //! Counters of one rendered frame, read back from the GPU a few frames later.
    struct Stats final
    {
        static constexpr uint32_t kMaxLodCount = 16;

        uint32_t m_instanceCandidates = 0;
        uint32_t m_instanceFrustumCulled = 0;
        uint32_t m_instanceOcclusionCulled = 0;
        uint32_t m_instancesRendered = 0;
        uint32_t m_instancesRenderedLate = 0;
        uint32_t m_meshletCandidates = 0;
        uint32_t m_meshletFrustumCulled = 0;
        uint32_t m_meshletOcclusionCulled = 0;
        uint32_t m_meshletsRendered = 0;
        uint32_t m_trianglesRendered = 0;
        //! Rendered instances per LOD; LOD zero is the most detailed one.
        uint32_t m_lodInstances[kMaxLodCount] = {};
        bool m_isValid = false;
    };


    //! Culling inputs of one view, published into the view blackboard.
    struct PassData final
    {
        Settings m_settings;
        //! Shadow views retain their light frustum while selecting LODs exactly as the camera view does.
        const PassData* m_lodReference = nullptr;
        float m_lodViewportHeight = 0.0f;
        //! Reuse instance classification across shadow levels and layers; meshlets still use this view's frustum and mask.
        const CulledMeshView* m_sharedInstances = nullptr;
        //! The camera used for frustum culling and LOD selection.
        Matrix4x4 m_viewProjection = Matrix4x4::kIdentity;
        //! The camera of the depth pyramid that occlusion culling tests against.
        Matrix4x4 m_occlusionViewProjection = Matrix4x4::kIdentity;
        Vector3 m_cameraPosition = Vector3::kZero;
        //! The vertical projection scale of the culling camera: the cotangent of half the vertical field of view, or two
        //! divided by the height of an orthographic projection.
        float m_projectionScaleY = 1.0f;
        //! Orthographic LOD selection ignores the distance to the camera.
        bool m_isOrthographic = false;
        //! Receives read back statistics when not null.
        Stats* m_stats = nullptr;
        //! Set while culling is frozen. The renderer captures the pyramid into it in the first frozen frame.
        Tools::DepthPyramid::Output* m_frozenDepthPyramid = nullptr;
        //! Occluders the view supplies itself, e.g. the clean pages of a shadow clip level.
        const Tools::DepthPyramid::Output* m_occluderPyramid = nullptr;

        //! The supplied or captured frozen pyramid; while it exists, views render once and test occlusion against it.
        [[nodiscard]] const Tools::DepthPyramid::Output* GetOccluderPyramid() const
        {
            if (m_occluderPyramid != nullptr)
                return m_occluderPyramid;

            const bool isCaptured = m_frozenDepthPyramid != nullptr && m_frozenDepthPyramid->m_texture != nullptr;
            return isCaptured ? m_frozenDepthPyramid : nullptr;
        }
    };


    //! Builds GPU inputs while keeping a shadow view's light frustum independent of its camera LOD reference.
    [[nodiscard]] MeshPass::MeshCullData BuildMeshCullData(const PassData& culling, float viewportHeight);


    //! Exposes culling settings and statistics of a view and can freeze its culling camera for inspection.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("E936245B-09F3-475C-A476-3568CE7268FC");

        Settings m_settings;

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

        [[nodiscard]] const Stats& GetStats() const
        {
            return m_stats;
        }

        [[nodiscard]] bool IsFrozen() const
        {
            return m_isFrozen;
        }

        //! The culling camera captured when the freeze started; valid while IsFrozen() returns true.
        [[nodiscard]] const Matrix4x4& GetFrozenViewProjection() const
        {
            return m_frozenViewProjection;
        }

    private:
        void DoRelease() override;

        Stats m_stats;
        Tools::DepthPyramid::Output m_frozenDepthPyramid;
        Matrix4x4 m_frozenViewProjection = Matrix4x4::kIdentity;
        Vector3 m_frozenCameraPosition = Vector3::kZero;
        float m_frozenProjectionScaleY = 1.0f;
        bool m_isFrozenOrthographic = false;
        bool m_isFrozen = false;
    };


    //! Return the culling inputs of the view; views without a culling module use default settings and their own camera.
    PassData& GetPassData(Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::Culling
