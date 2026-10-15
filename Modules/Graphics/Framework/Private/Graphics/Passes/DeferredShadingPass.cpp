#include <Core/Math/Colors.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/DeferredShadingPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Shaders/Deferred/Deferred.h>

namespace FE::Graphics::DeferredShading
{
    using namespace Visibility;

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(InitializeDeferredPipeline, "Shaders/Deferred/InitializeDeferred.cs.hlsl");
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
        blackboard.Add<PassData>().m_work = &m_work;
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        if (!blackboard.Contains<PassData>() || !blackboard.Contains<SurfaceEvaluation::Output>())
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        if (view.m_view->GetVisibilityDebugMode() != DebugMode::kDeferred)
            return;

        const auto& gBufferTargets = blackboard.Get<SurfaceEvaluation::Output>().m_gBufferTargets;

        FE_FG_SCOPE(graph, "DeferredShading");
        const Vector2UInt resolution(gBufferTargets[0]->GetDesc().m_width, gBufferTargets[0]->GetDesc().m_height);
        DeferredViewData constants{};
        constants.m_inverseViewProjection = view.m_view->GetInverseViewProjectionMatrix();

        const auto position = view.m_view->GetCameraTransform().Translation();
        constants.m_cameraPosition = PackedVector4F(position.x, position.y, position.z, 1);
        constants.m_viewport = PackedVector4F(view.m_viewportRect.left,
                                              view.m_viewportRect.top,
                                              view.m_viewportRect.Width(),
                                              view.m_viewportRect.Height());
        constants.m_backgroundColor = PackedVector4F(Vector4(Colors::kDarkSlateBlue));

        Rc<Core::Buffer> viewBuffer = Core::Buffer::CreateStructured<DeferredViewData>(graph.GetDevice(), "DeferredView", 1);
        FE_Verify(view.m_renderQueueUploader->Upload(graph, viewBuffer.Get(), constants));

        const MeshSceneModule* meshes = view.m_scene->GetModules().Contains<MeshSceneModule>()
            ? &view.m_scene->GetModules().Find<MeshSceneModule>()
            : nullptr;

        const uint32_t shaderCount = meshes ? meshes->GetShadingPipelines().GetPipelines().size() : 0;
        auto* initialize = graph.AllocatePassData<DeferredDirectPassDesc>();
        initialize->m_pipeline = InitializeDeferredPipeline::GetPipeline();
        initialize->m_constants.m_view = graph.GetSRV(viewBuffer.Get());
        initialize->m_constants.m_g0 = graph.GetSRV(Core::TextureView::Create(gBufferTargets[0].Get()));
        initialize->m_constants.m_output = graph.GetUAV(Core::TextureView::Create(view.m_mainColorTarget));
        initialize->m_constants.m_resolution = resolution;
        initialize->m_constants.m_shaderCount = shaderCount;
        graph.AddDispatchPass("InitializeDeferredColor", initialize, Math::CeilDivide(resolution, 8u));
        if (meshes == nullptr)
            return;

        SurfaceWork& work = *blackboard.Get<PassData>().m_work;
        {
            FE_FG_SCOPE(graph, "ShadingClassification");
            BuildSurfaceWork(graph, *view.m_renderQueueUploader, nullptr, nullptr, gBufferTargets[0].Get(), work, true);
        }

        const auto pipelines = meshes->GetShadingPipelines().GetPipelines();
        for (uint32_t slot = 0; slot < pipelines.size(); ++slot)
        {
            if (pipelines[slot] == nullptr)
                continue;

            auto* shading = graph.AllocatePassData<DeferredPassDesc>();
            shading->m_pipeline = pipelines[slot].Get();
            shading->m_arguments =
                Core::BufferView(work.m_arguments.Get()).Slice(slot * sizeof(DispatchArguments), sizeof(DispatchArguments));
            shading->m_constants.m_view = graph.GetSRV(viewBuffer.Get());
            shading->m_constants.m_g0 = graph.GetSRV(gBufferTargets[0].Get());
            shading->m_constants.m_g1 = graph.GetSRV(gBufferTargets[1].Get());
            shading->m_constants.m_g2 = graph.GetSRV(gBufferTargets[2].Get());
            shading->m_constants.m_g3 = graph.GetSRV(gBufferTargets[3].Get());
            shading->m_constants.m_depth = graph.GetSRV(view.m_mainDepthTarget);
            shading->m_constants.m_output = graph.GetUAV(view.m_mainColorTarget);
            shading->m_constants.m_pixels = graph.GetSRV(work.m_pixels.Get());
            shading->m_constants.m_waves = graph.GetSRV(work.m_waves.Get());
            shading->m_constants.m_ranges = graph.GetSRV(work.m_ranges.Get());
            shading->m_constants.m_resolution = resolution;
            shading->m_constants.m_tilesPerRow = Math::CeilDivide(resolution.x, kTileSize);
            shading->m_constants.m_shaderSlot = slot;
            shading->m_constants.m_shaderCount = pipelines.size();
            graph.AddPass("ShadeSurface", shading, [](Core::FrameGraphContext& context) {
                context.DispatchIndirect();
            });
        }
    }
} // namespace FE::Graphics::DeferredShading
