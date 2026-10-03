#pragma once
#include <Graphics/Core/GraphicsPipeline.h>
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct MeshPipelineSlot final
    {
        Rc<Core::GraphicsPipeline> m_pipeline;
        uint32_t m_instanceCapacity = 0;
        uint32_t m_workCapacity = 0;
    };


    struct MeshPipelineRegistry final
    {
        Env::Name m_techniqueRole;
        uint64_t m_revision = 0;
        festd::vector<MeshPipelineSlot> m_slots;
        Rc<Core::Buffer> m_routing;
    };


    struct MeshPipelineSubmission final
    {
        Rc<Core::GraphicsPipeline> m_pipeline;
        uint32_t m_firstCommand;
        uint32_t m_commandCount;
    };


    struct CulledMeshView final
    {
        MeshPass::ViewData m_viewData{};
        Rc<Core::Buffer> m_classification;
        uint32_t m_dispatchCount = 0;
    };


    struct PreparedMeshPass final
    {
        Rc<Core::Buffer> m_view;
        Rc<Core::Buffer> m_instances;
        Rc<Core::Buffer> m_work;
        Rc<Core::Buffer> m_commands;
        Rc<Core::Buffer> m_arguments;
        festd::vector<MeshPipelineSubmission> m_submissions;
    };


    namespace GpuMeshWorkBuilder
    {
        void CullView(Core::FrameGraph& graph, Core::RingUploader& uploader, festd::span<const MeshPass::CullDispatch> dispatches,
                      MeshPass::ViewData viewData, CulledMeshView& result);
        void Build(Core::FrameGraph& graph, Core::RingUploader& uploader, const MeshPipelineRegistry& registry,
                   const CulledMeshView& culled, Vector2UInt drawTagMask, PreparedMeshPass& result);
    } // namespace GpuMeshWorkBuilder
} // namespace FE::Graphics
