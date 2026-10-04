#include <Core/Base/PlatformInclude.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Math/Matrix4x4.h>
#include <Core/Threading/Thread.h>
#include <Core/Time/BaseTime.h>
#include <Framework/Application/Application.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <Graphics/Assets/Streamers.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Passes/DepthPrepass.h>
#include <Graphics/Passes/DrawTags.h>
#include <Graphics/Passes/OpaquePass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Tables/MeshMemberTable.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - GPU Driven Test App";
const IO::AssetID kBunnyModelAssetId("4B01B878-5EEF-4D6A-AD04-FC9AF1F78416");
const IO::AssetID kBunnyMaterialAssetId("EFA88960-2315-407E-BAAC-F88DFDF6FFBC");
const IO::AssetID kWarmBunnyMaterialAssetId("D5BA9140-9F93-4407-B5CE-E9FB01C2DDEC");
const IO::AssetID kCoolBunnyMaterialAssetId("FBD2877D-D6CB-4A11-A8A6-569E066A68EF");

namespace
{
    // Exercise the upper word of the mask with a statically allocated draw tag.
    const DrawTag GMaskProbeTag = [] {
        for (uint32_t index = 0; index < 32; ++index)
            FE::Graphics::Internal::GetNextDrawTagValue();
        return DrawTag(FE::Graphics::Internal::GetNextDrawTagValue());
    }();

    bool GStressMode = true;
    bool GBenchmarkMode = false;
    bool GFlightMode = false;
    bool GAllCulled = false;
    bool GSmallMeshes = false;
    bool GLargeMeshes = false;
    uint32_t GFrameLimit = 0;
    uint32_t GStressInstanceCount = 513;

    void ValidateMembershipResize(Core::Device* device, Core::ResourcePool* resourcePool)
    {
        DB::Database database(device, resourcePool);
        auto table = Rc<MeshMemberTable>(Memory::DefaultNew<MeshMemberTable>(&database));
        auto rows = table->AllocateRows(65);
        const uint32_t originalOffset = rows.m_rowIndex;
        table->WriteRow(originalOffset).m_instance.Get() = { 123 };

        // Grow and shrink within one allocation without losing existing references.
        FE_Verify(table->TryReallocateRows(rows, 127));
        FE_Assert(rows.m_rowIndex == originalOffset && rows.m_count == 127);
        FE_Assert(table->ReadRow(originalOffset).m_instance.Get().m_rowIndex == 123);
        FE_Assert(table->ReadRow(originalOffset + 126).m_instance.Get().m_rowIndex == 0);
        table->WriteRow(originalOffset + 126).m_instance.Get() = { 456 };
        FE_Verify(table->TryReallocateRows(rows, 65));
        FE_Verify(table->TryReallocateRows(rows, 127));
        FE_Assert(table->ReadRow(originalOffset + 126).m_instance.Get().m_rowIndex == 0);

        // Failed resizing must leave both the slice and its contents intact.
        const bool shrankAcrossSizeClass = table->TryReallocateRows(rows, 64);
        const bool grewAcrossSizeClass = table->TryReallocateRows(rows, 129);
        FE_Assert(!shrankAcrossSizeClass && !grewAcrossSizeClass);
        FE_Assert(rows.m_rowIndex == originalOffset && rows.m_count == 127);
        FE_Assert(table->ReadRow(originalOffset).m_instance.Get().m_rowIndex == 123);

        FE_Verify(table->TryReallocateRows(rows, 0));
        FE_Assert(rows.m_count == 0);
        const bool grewEmptySlice = table->TryReallocateRows(rows, 1);
        FE_Assert(!grewEmptySlice);
        table->Free(rows);

        // Exercise the maximum supported slice and reject growth beyond one page.
        rows = table->AllocateRows(MeshMemberTable::kRowsPerPage);
        const uint32_t lastRow = rows.m_rowIndex + rows.m_count - 1;
        table->WriteRow(lastRow).m_instance.Get() = { 789 };
        const bool exceededPageSize = table->TryReallocateRows(rows, MeshMemberTable::kRowsPerPage + 1);
        FE_Assert(!exceededPageSize);
        FE_Assert(rows.m_count == MeshMemberTable::kRowsPerPage);
        FE_Assert(table->ReadRow(lastRow).m_instance.Get().m_rowIndex == 789);
        table->Free(rows);
    }


    struct GpuDrivenTestApplication final : public Framework::Application
    {
        ~GpuDrivenTestApplication() override
        {
            m_device->WaitIdle();
            m_stressDesc = {};
            m_triangleRequest.Reset();

            m_scene->GetModules().Remove<MeshSceneModule>();
            m_secondView.Reset();
            m_view.Reset();
            m_scene.Reset();
            m_viewport.Reset();

            m_materialRequest.Reset();
            m_warmMaterialRequest.Reset();
            m_coolMaterialRequest.Reset();
            m_opaqueOnlyRequest.Reset();
            m_depthOnlyRequest.Reset();

            m_modelRequest.Reset();
            IO::AssetManager::Shutdown();

            Memory::DefaultDelete(m_materialStreamer);
            Memory::DefaultDelete(m_textureStreamer);
            Memory::DefaultDelete(m_meshStreamer);

            m_pipelineFactory.Reset();
            m_device.Reset();
            Renderer::Shutdown();
            Core::DeviceFactory::Shutdown();
        }

        void InitializeApp()
        {
            FE_PROFILER_ZONE();

            Core::DeviceFactory::Init(Core::GraphicsApi::kVulkan);

            auto& factory = Core::DeviceFactory::Get();
            for (const Core::AdapterInfo& adapterInfo : factory.EnumerateAdapters())
            {
                if (adapterInfo.m_kind == Core::AdapterKind::kDiscrete)
                {
                    m_device = factory.CreateDevice(adapterInfo.m_name);
                    break;
                }
            }

            Renderer::Init(m_device.Get());

            auto& renderer = Renderer::Get();
            Core::ResourcePool* resourcePool = renderer.GetResourcePool();
            Core::GraphicsQueue* graphicsQueue = renderer.GetGraphicsQueue();
            if (GStressMode)
                ValidateMembershipResize(m_device.Get(), resourcePool);

            IO::ArtifactStore::SetCatalogSource(FE_GPU_DRIVEN_TEST_APP_ASSET_DIR);
            IO::AssetManager::Init();
            m_meshStreamer =
                Memory::DefaultNew<MeshStreamer>(m_device.Get(), resourcePool, renderer.GetAsyncCopyQueue(), graphicsQueue);
            m_textureStreamer = Memory::DefaultNew<TextureStreamer>(m_device.Get(),
                                                                    resourcePool,
                                                                    renderer.GetAsyncCopyQueue(),
                                                                    graphicsQueue,
                                                                    renderer.GetDescriptorManager());
            IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<MeshAsset>(), m_meshStreamer);
            IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<TextureAsset>(), m_textureStreamer);

            const RectInt clientRect = m_mainWindow->GetClientRect();
            m_viewport = m_device->CreateViewport(resourcePool, graphicsQueue);
            m_viewport->Init(Core::ViewportDesc::Create(clientRect, m_mainWindow->GetNativeHandle().m_value));

            m_pipelineFactory = m_device->CreatePipelineFactory(renderer.GetDescriptorManager());
            m_scene = renderer.CreateScene();
            m_materialStreamer =
                Memory::DefaultNew<MaterialStreamer>(renderer.GetMaterialParameterAllocator(), m_pipelineFactory.Get());
            IO::AssetManager::RegisterStreamer(Rtti::GetTypeID<MaterialInstanceAsset>(), m_materialStreamer);
            Core::CompileGlobalPipelineSets(m_pipelineFactory.Get());
            Core::WaitForGlobalPipelineSets();

            m_scene->GetModules().Add<MeshSceneModule>();
            m_view = m_scene->CreateView();
            m_view->GetModules().Add<DepthPrepass::ViewModule>();
            m_view->GetModules().Add<OpaquePass::ViewModule>();

            const float aspectRatio = static_cast<float>(clientRect.Width()) / static_cast<float>(clientRect.Height());
            const Vector3 cameraPosition(0.0f, 3.0f, -8.0f);
            const Vector3 cameraTarget(0.0f, 0.75f, 0.0f);
            const Matrix4x4 cameraMatrix = Math::Invert(Matrix4x4::LookAt(cameraPosition, cameraTarget, Vector3::AxisY()));
            m_view->SetCameraTransform(Transform::Create(cameraPosition, Math::ExtractRotation(cameraMatrix), 1.0f));
            m_view->SetProjection(Constants::kPI * 0.3f, aspectRatio, 0.01f, 1000.0f);

            const IO::Link<ModelAsset> modelLink(kBunnyModelAssetId);
            m_modelRequest = IO::AssetManager::LoadAsset(modelLink);
            while (!m_modelRequest.IsCompleted())
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }
            FE_Assert(m_modelRequest.GetResult() == IO::AssetLoadResult::kSucceeded, "Failed to load bunny model asset");

            const IO::AssetHandle<ModelAsset> modelHandle(m_modelRequest.GetAssetSlot());
            const IO::AssetRead<ModelAsset> model = modelHandle.Read();
            FE_Assert(model && model->m_meshes.size() == 1);
            const IO::AssetLease<MeshAsset> meshLease(model->m_meshes[0].GetAssetHandle().GetAssetSlot());
            FE_Assert(meshLease.IsReady());
            if (!GStressMode || GLargeMeshes)
            {
                const auto mesh = meshLease.Read();
                const uint32_t residentLod = mesh->m_lodErrors.size() - 1;
                m_meshStreamer->SetResidentLod(*mesh.Get(), 0);
                while (mesh->m_residentLod != residentLod || mesh->m_currentOperation != nullptr)
                {
                    IO::AssetManager::Tick();
                    Threading::Sleep(1);
                }
            }
            IO::AssetLease<MeshAsset> triangleLease;
            if (GStressMode)
            {
                m_triangleRequest =
                    IO::AssetManager::LoadAsset(IO::Link<ModelAsset>(IO::AssetID("7511C08B-7226-42DA-AFD7-2921A7EDC06A")));
                while (!m_triangleRequest.IsCompleted())
                {
                    IO::AssetManager::Tick();
                    Threading::Sleep(1);
                }
                FE_Assert(m_triangleRequest.GetResult() == IO::AssetLoadResult::kSucceeded);
                const auto triangle = IO::AssetHandle<ModelAsset>(m_triangleRequest.GetAssetSlot()).Read();
                FE_Assert(triangle && triangle->m_meshes.size() == 1);
                triangleLease = IO::AssetLease<MeshAsset>(triangle->m_meshes[0].GetAssetHandle().GetAssetSlot());
            }

            m_materialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kBunnyMaterialAssetId));
            m_warmMaterialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kWarmBunnyMaterialAssetId));
            m_coolMaterialRequest = IO::AssetManager::LoadAsset(IO::Link<MaterialInstanceAsset>(kCoolBunnyMaterialAssetId));
            while (!m_materialRequest.IsCompleted() || !m_warmMaterialRequest.IsCompleted()
                   || !m_coolMaterialRequest.IsCompleted())
            {
                IO::AssetManager::Tick();
                Threading::Sleep(1);
            }

            FE_Assert(m_materialRequest.GetResult() == IO::AssetLoadResult::kSucceeded, "Failed to load bunny material instance");
            FE_Assert(m_warmMaterialRequest.GetResult() == IO::AssetLoadResult::kSucceeded,
                      "Failed to load warm bunny material instance");
            FE_Assert(m_coolMaterialRequest.GetResult() == IO::AssetLoadResult::kSucceeded,
                      "Failed to load cool bunny material instance");
            if (GStressMode)
            {
                m_opaqueOnlyRequest = IO::AssetManager::LoadAsset(IO::AssetID("6C22D94D-661E-45CE-8AA3-F55783855D58"));
                m_depthOnlyRequest = IO::AssetManager::LoadAsset(IO::AssetID("FDB669B8-4BC3-4DB9-82B2-7CF7D47B68D1"));
                while (!m_opaqueOnlyRequest.IsCompleted() || !m_depthOnlyRequest.IsCompleted())
                {
                    IO::AssetManager::Tick();
                    Threading::Sleep(1);
                }
                FE_Assert(m_opaqueOnlyRequest.GetResult() == IO::AssetLoadResult::kSucceeded);
                FE_Assert(m_depthOnlyRequest.GetResult() == IO::AssetLoadResult::kSucceeded);
            }
            const IO::AssetRead<MaterialInstanceAsset> materialInstance =
                IO::AssetHandle<MaterialInstanceAsset>(m_materialRequest.GetAssetSlot()).Read();
            FE_Assert(materialInstance && materialInstance->m_runtime);
            for (const MaterialParameterValue& parameter : materialInstance->m_parameters)
            {
                if (!parameter.m_texture.GetAssetID().IsValid())
                    continue;

                const IO::AssetRead<TextureAsset> texture = parameter.m_texture.GetAssetHandle().Read();
                FE_Assert(texture);
                m_textureStreamer->SetResidentMip(*texture.Get(), 1);
            }
            auto& meshSceneModule = m_scene->GetModules().Find<MeshSceneModule>();
            MeshInstanceDesc instanceDesc;
            instanceDesc.m_asset = meshLease;
            const IO::AssetLease<MaterialInstanceAsset> originalMaterial(m_materialRequest.GetAssetSlot());
            const IO::AssetLease<MaterialInstanceAsset> warmMaterial(m_warmMaterialRequest.GetAssetSlot());
            const IO::AssetLease<MaterialInstanceAsset> coolMaterial(m_coolMaterialRequest.GetAssetSlot());
            for (int32_t batchIndex = -1; batchIndex <= 1; ++batchIndex)
            {
                instanceDesc.m_material = batchIndex < 0 ? warmMaterial : batchIndex > 0 ? coolMaterial : originalMaterial;
                const float batchX = static_cast<float>(batchIndex) * 2.5f;
                MeshBatchDesc batchDesc;
                batchDesc.m_bounds = Aabb{ Vector3(batchX - 5.0f, -5.0f, -5.0f), Vector3(batchX + 5.0f, 5.0f, 5.0f) };
                batchDesc.m_drawTagMask = DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque);
                instanceDesc.m_batch = meshSceneModule.CreateBatch(batchDesc);
                if (batchIndex == -1)
                    m_migrationBatch = instanceDesc.m_batch;

                for (int32_t instanceIndex = -1; instanceIndex <= 1; ++instanceIndex)
                {
                    instanceDesc.m_transform = Matrix4x4::RotationY(Constants::kPI)
                        * Matrix4x4::Translation(Vector3(batchX, 0.0f, static_cast<float>(instanceIndex) * 1.5f));
                    m_meshInstances.push_back(meshSceneModule.CreateInstance(instanceDesc));
                }
            }

            if (GStressMode)
            {
                FE_Assert(GMaskProbeTag.m_value >= 32);
                FE_Assert(warmMaterial.Read()->m_runtime->HasTechnique("MaskProbe"));
                FE_Assert(coolMaterial.Read()->m_runtime->HasTechnique("MaskProbe"));
                FE_Assert(!originalMaterial.Read()->m_runtime->HasTechnique("MaskProbe"));

                MeshBatchDesc stressBatch;
                stressBatch.m_bounds = Aabb{ Vector3(-200.0f), Vector3(200.0f) };
                stressBatch.m_drawTagMask =
                    DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque) | DrawTagMask(GMaskProbeTag);
                m_slotTestBatch = meshSceneModule.CreateBatch(stressBatch);
                m_slotTestIndex = m_slotTestBatch->m_octreeEntry.m_userIndex;
                m_stressBatch = meshSceneModule.CreateBatch(stressBatch);
                instanceDesc.m_batch = m_stressBatch;
                for (uint32_t index = 0; index < GStressInstanceCount; ++index)
                {
                    instanceDesc.m_asset = GLargeMeshes || (!GSmallMeshes && index % 2 == 0) ? meshLease : triangleLease;
                    instanceDesc.m_material = index % 3 == 0 ? originalMaterial : index % 3 == 1 ? warmMaterial : coolMaterial;
                    const float x = float(index % 27) * 2.0f - 26.0f;
                    const float z = float(index / 27) * 2.0f;
                    instanceDesc.m_transform = Matrix4x4::Translation(Vector3(x, 0.0f, z));
                    m_stressInstances.push_back(meshSceneModule.CreateInstance(instanceDesc));
                }
                instanceDesc.m_material = originalMaterial;
                m_stressDesc = instanceDesc;
                m_secondView = m_scene->CreateView();
                m_secondView->GetModules().Add<DepthPrepass::ViewModule>();
                m_secondView->GetModules().Add<OpaquePass::ViewModule>();
                auto& probePass = m_secondView->GetModules().Find<OpaquePass::ViewModule>();
                probePass.m_drawTag = GMaskProbeTag;
                probePass.m_techniqueRole = "MaskProbe";
                m_secondView->SetCameraTransform(Transform::Create(cameraPosition, Math::ExtractRotation(cameraMatrix), 1.0f));
                m_secondView->SetProjection(Constants::kPI * 0.2f, aspectRatio, 0.01f, 1000.0f);
            }

            MeshBatchDesc culledBatchDesc;
            instanceDesc.m_material = originalMaterial;
            culledBatchDesc.m_bounds = Aabb{ Vector3(95.0f, -5.0f, -5.0f), Vector3(105.0f, 5.0f, 5.0f) };
            culledBatchDesc.m_drawTagMask = DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque);
            instanceDesc.m_batch = meshSceneModule.CreateBatch(culledBatchDesc);
            instanceDesc.m_transform = Matrix4x4::Translation(Vector3(100.0f, 0.0f, 0.0f));
            m_meshInstances.push_back(meshSceneModule.CreateInstance(instanceDesc));
            if (GAllCulled)
            {
                const Matrix4x4 outside = Matrix4x4::Translation(Vector3(100.0f, 100.0f, -100.0f));
                for (MeshHandle handle : m_meshInstances)
                    meshSceneModule.UpdateTransform(handle, outside);
                for (MeshHandle handle : m_stressInstances)
                    meshSceneModule.UpdateTransform(handle, outside);
            }
        }

        Rc<WaitGroup> ScheduleUpdate() override
        {
            FE_PROFILER_ZONE();
            IO::AssetManager::Tick();
            if (GStressMode && !GBenchmarkMode)
            {
                auto& scene = m_scene->GetModules().Find<MeshSceneModule>();
                if (m_frameIndex == 2)
                {
                    // Deleting a middle slot must preserve every surviving batch index.
                    const MeshBatch* lastBatch = scene.GetBatches().back();
                    const uint32_t lastIndex = lastBatch->m_octreeEntry.m_userIndex;
                    scene.DestroyBatch(m_slotTestBatch);
                    m_slotTestBatch = nullptr;
                    FE_Assert(scene.GetBatches()[m_slotTestIndex] == nullptr);
                    FE_Assert(lastBatch->m_octreeEntry.m_userIndex == lastIndex);
                    FE_Assert(scene.GetBatches()[lastIndex] == lastBatch);

                    for (uint32_t index = 0; index < 257; ++index)
                        scene.DestroyInstance(m_stressInstances[index]);
                }
                if (m_frameIndex == 3)
                {
                    MeshBatchDesc batchDesc;
                    batchDesc.m_bounds = Aabb{ Vector3(-200.0f), Vector3(200.0f) };
                    m_slotTestBatch = scene.CreateBatch(batchDesc);
                    const uint32_t newIndex = m_slotTestBatch->m_tableRef.m_rowIndex;
                    FE_Assert(m_slotTestBatch->m_octreeEntry.m_userIndex == newIndex);
                    FE_Assert(scene.GetBatches()[newIndex] == m_slotTestBatch);
                    if (newIndex != m_slotTestIndex)
                        FE_Assert(scene.GetBatches()[m_slotTestIndex] == nullptr);

                    for (uint32_t index = 0; index < 257; ++index)
                    {
                        m_stressDesc.m_transform =
                            Matrix4x4::Translation(Vector3(float(index % 19) - 9.0f, 0.0f, float(index / 19)));
                        m_stressInstances[index] = scene.CreateInstance(m_stressDesc);
                    }
                }
                if (m_frameIndex == 4)
                {
                    // Discard pending dirty bits before the scene update, leaving an unused batch row.
                    MeshBatchDesc transientDesc;
                    transientDesc.m_bounds = Aabb{ Vector3(-1.0f), Vector3(1.0f) };
                    scene.DestroyBatch(scene.CreateBatch(transientDesc));

                    scene.SetBatchDrawTags(m_stressBatch, DrawTagMask(DrawTags::Opaque));
                    Matrix4x4 shear = Matrix4x4::Translation(Vector3(0.0f, 0.0f, 3.0f));
                    shear.m_00 = -1.5f;
                    shear.m_11 = 0.25f;
                    shear.m_22 = 2.0f;
                    shear.m_10 = 0.5f;
                    scene.UpdateTransform(m_stressInstances[0], shear);
                    shear.m_22 = 0.0f;
                    scene.UpdateTransform(m_stressInstances[1], shear);
                    scene.UpdateMaterial(m_stressInstances[2],
                                         IO::AssetLease<MaterialInstanceAsset>(m_warmMaterialRequest.GetAssetSlot()));
                    scene.UpdateTransform(m_stressInstances.back(), Matrix4x4::Translation(Vector3(100.0f, 0.0f, 0.0f)));
                }
                if (m_frameIndex == 5)
                {
                    scene.SetBatchDrawTags(m_stressBatch, DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(GMaskProbeTag));
                    m_view->GetModules().Deactivate<OpaquePass::ViewModule>();
                }
                if (m_frameIndex == 6)
                    m_view->GetModules().Activate<OpaquePass::ViewModule>();
                if (m_frameIndex == 7)
                {
                    scene.SetBatchDrawTags(m_stressBatch,
                                           DrawTagMask(DrawTags::DepthPrepass) | DrawTagMask(DrawTags::Opaque)
                                               | DrawTagMask(GMaskProbeTag));
                    scene.MoveInstance(m_stressInstances.back(), m_migrationBatch);
                }
                if (m_frameIndex == 8 || m_frameIndex == 9)
                {
                    const auto model = IO::AssetHandle<ModelAsset>(m_modelRequest.GetAssetSlot()).Read();
                    const auto mesh = model->m_meshes[0].GetAssetHandle().Read();
                    const uint32_t lod = m_frameIndex == 8 ? 0 : mesh->m_lodErrors.size() - 1;
                    const uint32_t residentLod = mesh->m_lodErrors.size() - lod - 1;
                    m_meshStreamer->SetResidentLod(*mesh.Get(), lod);
                    while (mesh->m_residentLod != residentLod || mesh->m_currentOperation != nullptr)
                    {
                        IO::AssetManager::Tick();
                        Threading::Sleep(1);
                    }
                }
#if FE_DEVELOPMENT
                if (m_frameIndex == 10)
                {
                    const auto model = IO::AssetHandle<ModelAsset>(m_modelRequest.GetAssetSlot()).Read();
                    auto meshReload = IO::AssetManager::ReloadAsset(model->m_meshes[0].GetAssetID());
                    auto materialReload = IO::AssetManager::ReloadAsset(kWarmBunnyMaterialAssetId);
                    FE_Assert(meshReload.IsValid() && materialReload.IsValid());
                    while (!meshReload.IsCompleted() || !materialReload.IsCompleted())
                    {
                        IO::AssetManager::Tick();
                        Threading::Sleep(1);
                    }
                    FE_Assert(meshReload.GetResult() == IO::AssetLoadResult::kSucceeded);
                    FE_Assert(materialReload.GetResult() == IO::AssetLoadResult::kSucceeded);
                }
#endif
                if (m_frameIndex == 11)
                {
                    scene.UpdateMaterial(m_stressInstances[0],
                                         IO::AssetLease<MaterialInstanceAsset>(m_opaqueOnlyRequest.GetAssetSlot()));
                    scene.UpdateMaterial(m_stressInstances[1],
                                         IO::AssetLease<MaterialInstanceAsset>(m_depthOnlyRequest.GetAssetSlot()));
                }
                if (m_frameIndex == 12)
                {
                    const Matrix4x4 outside = Matrix4x4::Translation(Vector3(100.0f, 100.0f, -100.0f));
                    for (MeshHandle handle : m_meshInstances)
                        scene.UpdateTransform(handle, outside);
                    for (MeshHandle handle : m_stressInstances)
                        scene.UpdateTransform(handle, outside);
                }
                if (m_frameIndex == 13)
                {
                    scene.DestroyBatch(m_slotTestBatch);
                    m_slotTestBatch = nullptr;

                    for (MeshHandle handle : m_stressInstances)
                        scene.DestroyInstance(handle);
                    for (MeshHandle handle : m_meshInstances)
                        scene.DestroyInstance(handle);
                }
            }
            HighResolutionTimer frameTimer;
            if (GBenchmarkMode)
                frameTimer.Start();
            Renderer::Get().Render(m_scene.Get(), m_viewport.Get());
            if (GStressMode)
            {
                const auto& scene = m_scene->GetModules().Find<MeshSceneModule>();
                for (const MeshBatch* batch : scene.GetBatches())
                {
                    if (batch != nullptr)
                        FE_Assert(batch->m_members.m_count == batch->m_meshInstances.size());
                }
            }
            if (GStressMode && !GFlightMode)
                m_device->WaitIdle();
            if (GBenchmarkMode)
            {
                frameTimer.Stop();
                if (m_frameIndex >= 4)
                {
                    m_frameMilliseconds += frameTimer.GetElapsedMilliseconds();
                    ++m_timingSamples;
                }
            }
            ++m_frameIndex;
#if FE_PLATFORM_WINDOWS
            if (GFrameLimit != 0 && m_frameIndex >= GFrameLimit)
            {
                if (GBenchmarkMode)
                {
                    if (m_timingSamples != 0)
                    {
                        Logger::LogInfo("Renderer frame time (CPU submission + GPU wait): {} ms over {} frames",
                                        m_frameMilliseconds / m_timingSamples,
                                        m_timingSamples);
                    }
                }
                PostMessageW(reinterpret_cast<HWND>(m_mainWindow->GetNativeHandle().m_value), WM_CLOSE, 0, 0);
            }
#endif
            return nullptr;
        }

        void DoRelease() override
        {
            this->~GpuDrivenTestApplication();
        }

        Rc<Core::Device> m_device;
        Rc<Core::Viewport> m_viewport;
        Rc<Core::PipelineFactory> m_pipelineFactory;

        Rc<Scene> m_scene;
        Rc<View> m_view;

        IO::AssetRequest m_modelRequest;
        IO::AssetRequest m_triangleRequest;
        IO::AssetRequest m_materialRequest;
        IO::AssetRequest m_warmMaterialRequest;
        IO::AssetRequest m_coolMaterialRequest;
        IO::AssetRequest m_opaqueOnlyRequest;
        IO::AssetRequest m_depthOnlyRequest;
        MaterialStreamer* m_materialStreamer = nullptr;
        MeshStreamer* m_meshStreamer = nullptr;
        TextureStreamer* m_textureStreamer = nullptr;
        festd::vector<MeshHandle> m_meshInstances;
        festd::vector<MeshHandle> m_stressInstances;
        MeshBatch* m_migrationBatch = nullptr;
        MeshBatch* m_slotTestBatch = nullptr;
        uint32_t m_slotTestIndex = kInvalidIndex;
        MeshBatch* m_stressBatch = nullptr;
        MeshInstanceDesc m_stressDesc;
        Rc<View> m_secondView;
        uint32_t m_frameIndex = 0;
        double m_frameMilliseconds = 0.0;
        uint32_t m_timingSamples = 0;
    };
} // namespace

int main(const int32_t argc, const char** argv)
{
    for (int32_t index = 1; index < argc; ++index)
    {
        if (strcmp(argv[index], "--gpu-stress") == 0)
            GStressMode = true;
        if (strcmp(argv[index], "--gpu-benchmark") == 0)
        {
            GStressMode = true;
            GBenchmarkMode = true;
        }
        if (strcmp(argv[index], "--gpu-flight-stress") == 0)
        {
            GStressMode = true;
            GFlightMode = true;
        }
        if (strcmp(argv[index], "--gpu-all-culled") == 0)
            GAllCulled = true;
        if (strcmp(argv[index], "--gpu-small-meshes") == 0)
            GSmallMeshes = true;
        if (strcmp(argv[index], "--gpu-large-meshes") == 0)
            GLargeMeshes = true;
        if (strcmp(argv[index], "--instances") == 0 && index + 1 < argc)
            GStressInstanceCount = Math::Clamp(static_cast<uint32_t>(atoi(argv[++index])), 257u, MeshBatch::kMaxInstanceCount);
        if (strcmp(argv[index], "--frames") == 0 && index + 1 < argc)
            GFrameLimit = static_cast<uint32_t>(atoi(argv[++index]));
    }
    Env::ApplicationInfo applicationInfo;
    if (GStressMode && GFrameLimit == 0)
        GFrameLimit = GBenchmarkMode ? 64 : 15;
    applicationInfo.m_name = kExampleName;

    Env::Init(applicationInfo, argc, argv);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
    auto* application = Memory::New<GpuDrivenTestApplication>(allocator);

    struct ApplicationJob final : public Jobs::JobNode
    {
        void Execute() override
        {
            m_application->InitializeWindow();
            m_application->InitializeApp();
            m_exitCode = m_application->Run();
        }

        GpuDrivenTestApplication* m_application = nullptr;
        int32_t m_exitCode = 0;
    };
    // The application stops the scheduler while this launch job is suspended.
    ApplicationJob applicationJob;
    applicationJob.m_application = application;
    applicationJob.Dispatch(Jobs::FiberAffinityMask::kMainThread);

    Jobs::StartJobSystem();

    Memory::Delete(allocator, application);
    Env::Shutdown();
    return applicationJob.m_exitCode;
}
