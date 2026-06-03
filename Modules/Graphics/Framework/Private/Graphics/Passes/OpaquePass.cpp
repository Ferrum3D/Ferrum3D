#include <FeCore/Math/Colors.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

#include <Shaders/Passes/OpaquePass/OpaquePass.h>

namespace FE::Graphics::OpaquePass
{
    namespace
    {
        struct Pipeline final : public Core::GraphicsPipelineVariantSet
        {
            FE_SHADER_SPEC(ColorTargetFormat, Core::Format::kB8G8R8A8_SRGB, Core::Format::kR8G8B8A8_SRGB,
                           Core::Format::kR8G8B8A8_UNORM, Core::Format::kB8G8R8A8_UNORM);
            using Specializer = Core::ShaderSpecializer<ColorTargetFormat>;

            FE_DECLARE_PIPELINE_SET(Pipeline, Specializer);

        private:
            void SetupRequest(const uint32_t variantIndex, Core::GraphicsPipelineRequest& request) override
            {
                const Specializer specializer(variantIndex);

                request.m_desc.SetMeshShader("Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl")
                    .SetPixelShader("Shaders/Passes/OpaquePass/OpaquePass.ps.hlsl")
                    .SetRTVFormat(specializer.Get<ColorTargetFormat>())
                    .SetDSVFormat(Core::Format::kD32_SFLOAT_S8_UINT)
                    .SetDepthStencil(Core::DepthStencilState::kWriteIfEqual)
                    .SetColorBlend(Core::TargetColorBlending::kDisabled)
                    .SetRasterization(Core::RasterizationState::kFillBackCull);
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(Pipeline);
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        blackboard.Add<PassData>();
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene)
    {
        if (!blackboard.Contains<PassData>())
            return;

        const RendererViewData& viewData = blackboard.Get<RendererViewData>();
        auto* meshModule = scene.GetModules().TryFind<MeshSceneModule>();
        if (meshModule == nullptr)
            return;

        MeshBatch* batch = festd::single(meshModule->GetBatches());
        FE_Assert(batch != nullptr);

        const DB::Ref<MeshInstanceTable> instanceRef = festd::single(batch->m_meshInstances);

        const MeshInstanceTable::Row instanceRow = meshModule->GetMeshInstanceTable()->ReadRow(instanceRef);
        const MeshGroupTable::Row groupRow = meshModule->GetMeshGroupTable()->ReadRow(instanceRow.m_meshGroup.Get());
        ModelAsset* modelAsset = meshModule->FindAsset(instanceRow.m_meshGroup.Get());
        FE_Assert(modelAsset);

        const DB::Slice<MeshLodInfoTable> lods = groupRow.m_lods.Get();
        const Core::MeshLodInfo lodInfo = meshModule->GetMeshLodInfoTable()->ReadRow(lods.m_rowIndex).m_info.Get();

        Pipeline::Specializer specializer;
        specializer.Set<Pipeline::ColorTargetFormat>(viewData.m_mainColorTarget->GetDesc().m_imageFormat);

        auto* passDesc = graph.AllocatePassData<PassDesc>();
        passDesc->m_constants.m_meshInstanceTable = meshModule->GetMeshInstanceTable()->GetDeviceAddress();
        passDesc->m_constants.m_meshGroupTable = meshModule->GetMeshGroupTable()->GetDeviceAddress();
        passDesc->m_constants.m_meshLodInfoTable = meshModule->GetMeshLodInfoTable()->GetDeviceAddress();
        passDesc->m_constants.m_instanceIndex = instanceRef.m_rowIndex;
        passDesc->m_constants.m_viewProjection = viewData.m_view->GetViewProjectionMatrix();
        passDesc->m_constants.m_baseColor = float4(0.78f, 0.74f, 0.68f, 1.0f);
        passDesc->m_geometryBuffer = {
            Core::BufferView::Create(modelAsset->GetGeometryBuffer(0)),
            Core::BarrierSyncFlags::kMeshShading,
            Core::BarrierAccessFlags::kShaderRead,
        };
        passDesc->m_colorTarget = Core::TextureView::Create(viewData.m_mainColorTarget);
        passDesc->m_depthTarget = Core::TextureView::Create(viewData.m_mainDepthTarget);
        passDesc->m_viewport = viewData.m_viewportRect;
        passDesc->m_pipeline = Pipeline::GetPipeline(specializer);

        const bool hasDepthPrepass = blackboard.TryGet<DepthPrepass::PassData>() != nullptr;
        graph.AddPass("OpaquePass",
                      passDesc,
                      [meshletCount = lodInfo.m_meshletCount, hasDepthPrepass](Core::FrameGraphContext& context) {
                          context.ClearColorTarget(0, Colors::kDarkSlateBlue);
                          if (!hasDepthPrepass)
                              context.ClearDepthStencilTarget(0.0f, 0);

                          context.DispatchMesh(meshletCount);
                      });
    }
} // namespace FE::Graphics::OpaquePass
