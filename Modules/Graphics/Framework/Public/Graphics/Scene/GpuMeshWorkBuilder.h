#pragma once
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <festd/fixed_function.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct MeshPipelineSlot final
    {
        Rc<Core::GraphicsPipeline> m_pipeline;
        uint32_t m_instanceCapacity = 0;
        uint32_t m_workCapacity = 0;
    };


    struct MeshPipelineSubmission final
    {
        Rc<Core::GraphicsPipeline> m_pipeline;
        uint32_t m_firstCommand;
        uint32_t m_commandCount;
    };


    struct MeshPipelineRegistry final
    {
        Env::Name m_techniqueRole;
        uint32_t m_revision = 0;
        festd::vector<MeshPipelineSlot> m_slots;
        festd::vector<MeshPipelineSubmission> m_submissions;
        Rc<Core::Buffer> m_routing;
        Rc<Core::Buffer> m_layouts;
        uint32_t m_instanceCapacity = 0;
        uint32_t m_workCapacity = 0;
        uint32_t m_commandCapacity = 0;
    };


    //! Persistent and per-frame resources that culling of one view reads and writes.
    struct MeshCullViewResources final
    {
        //! One MeshPass::InstanceVisibility word per instance row.
        Core::Buffer* m_visibility = nullptr;
        uint32_t m_visibilityCapacity = 0;
        //! One bit per meshlet of each instance, at the word offset stored in its instance row.
        Core::Buffer* m_meshletVisibility = nullptr;
        uint32_t m_meshletVisibilityCapacity = 0;
        //! MeshPass::CullStat counters of this frame.
        Core::Buffer* m_stats = nullptr;
        //! A frozen depth pyramid tested in the single phase; null otherwise.
        const Tools::DepthPyramid::Output* m_depthPyramid = nullptr;
    };


    struct CulledMeshView final
    {
        MeshPass::MeshViewData m_viewData{};
        Rc<Core::Buffer> m_view;
        Rc<Core::Buffer> m_dispatches;
        Rc<Core::Buffer> m_visibility;
        Rc<Core::Buffer> m_meshletVisibility;
        Rc<Core::Buffer> m_stats;
        //! Instance classification of the single or early phase, and of the late phase once it was culled.
        Rc<Core::Buffer> m_classification;
        Rc<Core::Buffer> m_lateClassification;
        uint32_t m_visibilityCapacity = 0;
        uint32_t m_meshletVisibilityCapacity = 0;
        uint32_t m_dispatchCount = 0;
        uint32_t m_phase = MeshPass::CullPhase::kSingle;
    };


    struct PreparedMeshPass final
    {
        Rc<Core::Buffer> m_view;
        Rc<Core::Buffer> m_meshlets;
        Rc<Core::Buffer> m_commands;
        Rc<Core::Buffer> m_arguments;
        festd::vector<MeshPipelineSubmission> m_submissions;
    };


    //! The inputs of one rendering phase of a mesh pass.
    struct MeshPassPhase final
    {
        uint32_t m_phase = MeshPass::CullPhase::kSingle;
        //! Built from the early depth in the late phase, or frozen in the single phase; occlusion is tested when set.
        const Tools::DepthPyramid::Output* m_depthPyramid = nullptr;
        //! Receives meshlet and rendered instance counters when not null.
        Core::Buffer* m_stats = nullptr;
    };


    namespace GpuMeshWorkBuilder
    {
        //! Upload the view's cull inputs and classify instances for the single or early phase.
        void CullView(Core::FrameGraph& graph, Core::RingUploader& uploader, uint32_t dispatchCount,
                      const festd::fixed_function<48, void(festd::span<MeshPass::CullDispatch>)>& writeDispatches,
                      MeshPass::MeshViewData viewData, uint32_t phase, const MeshCullViewResources& resources,
                      CulledMeshView& result);

        //! Classify the instances for the late phase and record their occlusion results for the next frame.
        void CullLateInstances(Core::FrameGraph& graph, Core::RingUploader& uploader,
                               const Tools::DepthPyramid::Output& depthPyramid, CulledMeshView& culled);

        //! Bucket the phase's instances by pipeline, cull their meshlets and build the mesh shader dispatches.
        void Build(Core::FrameGraph& graph, Core::RingUploader& uploader, const MeshPipelineRegistry& registry,
                   const CulledMeshView& culled, const MeshPassPhase& phase, Vector2UInt drawTagMask, PreparedMeshPass& result);
    } // namespace GpuMeshWorkBuilder
} // namespace FE::Graphics
