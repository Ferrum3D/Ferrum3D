#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>
#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Tables/MaterialInstanceTable.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

#include <Shaders/Passes/MeshPass/IndirectArguments.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>

namespace FE::Graphics
{
    static void FreeInstanceParameters(const IO::AssetLease<MaterialInstanceAsset>& materialLease, const uint32_t generation,
                                       const MaterialParameterAllocator::Allocation& parameters)
    {
        // An older generation's runtime frees its remaining blocks on destruction; never free them through its replacement.
        if (generation != materialLease.GetGeneration() || !parameters.IsValid())
            return;

        const IO::AssetRead<MaterialInstanceAsset> material = materialLease.Read();
        if (material && material->m_runtime)
            material->m_runtime->FreeInstanceParameters(parameters);
    }


    MeshSceneModule::MeshSceneModule(Scene* scene)
        : SceneModuleBase(scene)
        , m_octree(Aabb{ Vector3(-10000.0f, -10000.0f, -10000.0f), Vector3(10000.0f, 10000.0f, 10000.0f) })
    {
        RendererImpl* renderer = Rtti::AssertCast<RendererImpl*>(scene->GetRenderer());
        DB::Database* database = renderer->GetDatabase();
        FE_Assert(database != nullptr);

        m_meshLodInfoTable = Memory::DefaultNew<MeshLodInfoTable>(database);
        m_meshGroupTable = Memory::DefaultNew<MeshGroupTable>(database);
        m_meshInstanceTable = Memory::DefaultNew<MeshInstanceTable>(database);
        m_materialInstanceTable = Memory::DefaultNew<MaterialInstanceTable>(database);
        m_batchTable = Memory::DefaultNew<MeshBatchTable>(database);
        m_memberTable = Memory::DefaultNew<MeshMemberTable>(database);
    }


    MeshSceneModule::~MeshSceneModule()
    {
        for (MeshBatch* batch : m_batches)
        {
            if (batch != nullptr)
                DestroyBatch(batch);
        }

        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            DestroyGroup(m_meshGroups[groupIndex]);
        });
    }


    MeshBatch* MeshSceneModule::CreateBatch(const MeshBatchDesc& desc)
    {
        FE_Assert(desc.m_bounds.IsValid());
        auto* batch = Memory::DefaultNew<MeshBatch>();
        batch->m_parent = this;
        batch->m_tableRef = m_batchTable->AllocateRow();
        batch->m_drawTagMask = desc.m_drawTagMask;
        batch->m_octreeEntry.m_bounds = desc.m_bounds;
        const uint32_t batchIndex = batch->m_tableRef.m_rowIndex;
        if (batchIndex >= m_batches.size())
        {
            m_batches.resize(batchIndex + 1, nullptr);
            m_boundsDirty.resize(m_batches.size(), false);
            m_membershipDirty.resize(m_batches.size(), false);
        }
        m_batches[batchIndex] = batch;
        m_boundsDirty.set(batchIndex);
        m_membershipDirty.set(batchIndex);

        batch->m_octreeEntry.m_userIndex = batchIndex;
        m_octree.InsertOrUpdate(batch->m_octreeEntry);
        return batch;
    }


    void MeshSceneModule::DestroyBatch(MeshBatch* batch)
    {
        if (batch == nullptr)
            return;

        const uint32_t batchIndex = batch->m_tableRef.m_rowIndex;
        if (batchIndex >= m_batches.size() || m_batches[batchIndex] != batch)
            return;

        while (!batch->m_handles.empty())
            DestroyInstance(batch->m_handles.back());

        m_memberTable->Free(batch->m_members);
        m_batchTable->Free(batch->m_tableRef);

        m_octree.Remove(batch->m_octreeEntry);
        m_batches[batchIndex] = nullptr;
        m_boundsDirty.reset(batchIndex);
        m_membershipDirty.reset(batchIndex);

        Memory::DefaultDelete(batch);
    }


    MeshHandle MeshSceneModule::CreateInstance(const MeshInstanceDesc& desc)
    {
        FE_Assert(desc.m_asset.IsValid() && desc.m_asset.IsReady());
        FE_Assert(desc.m_material.IsValid() && desc.m_material.IsReady());
        FE_Assert(desc.m_batch != nullptr && desc.m_batch->m_parent == this);
        FE_Assert(desc.m_batch->m_meshInstances.size() < MeshBatch::kMaxInstanceCount);

        MeshGroup* meshGroup = FindOrCreateMeshGroup(desc.m_asset, desc.m_material);
        const IO::AssetRead<MaterialInstanceAsset> material = desc.m_material.Read();
        FE_Assert(material && material->m_runtime);
        const MaterialParameterAllocator::Allocation parameters = material->m_runtime->AllocateInstanceParameters();

        const DB::Ref<MeshInstanceTable> instanceRef = m_meshInstanceTable->AllocateRow();
        const MeshHandle handle = AllocateHandle(instanceRef, desc.m_batch, meshGroup, parameters);

        const MeshInstanceTable::RWRow instance = m_meshInstanceTable->WriteRow(instanceRef);
        instance.m_meshGroup.Get() = meshGroup->m_tableRef;
        instance.m_transform.Get() = desc.m_transform;
        instance.m_instanceData.Get() = parameters.m_devicePointer;

        desc.m_batch->m_meshInstances.push_back(instanceRef);
        desc.m_batch->m_handles.push_back(handle);
        meshGroup->m_instances.push_back(handle);
        ++meshGroup->m_instanceCount;
        m_membershipDirty.set(desc.m_batch->m_tableRef.m_rowIndex);
        m_boundsDirty.set(desc.m_batch->m_tableRef.m_rowIndex);
        ++m_revision;

        return handle;
    }


    void MeshSceneModule::DestroyInstance(const MeshHandle instance)
    {
        if (!instance.IsValid() || instance.m_value >= m_instances.size())
            return;

        const InstanceRecord& record = m_instances[instance.m_value];
        if (record.m_generation != instance.m_generation || record.m_tableRef.m_rowIndex == kInvalidIndex)
            return;

        FreeInstanceParameters(record.m_group->m_material, record.m_materialGeneration, record.m_parameters);

        const auto batchRef = festd::find_if(record.m_batch->m_meshInstances, [&](const DB::Ref<MeshInstanceTable> ref) {
            return ref.m_rowIndex == record.m_tableRef.m_rowIndex;
        });
        FE_Assert(batchRef != record.m_batch->m_meshInstances.end());
        record.m_batch->m_meshInstances.erase_unsorted(batchRef);
        const auto batchHandle = festd::find(record.m_batch->m_handles, instance);
        FE_Assert(batchHandle != record.m_batch->m_handles.end());
        record.m_batch->m_handles.erase_unsorted(batchHandle);
        m_membershipDirty.set(record.m_batch->m_tableRef.m_rowIndex);
        m_boundsDirty.set(record.m_batch->m_tableRef.m_rowIndex);
        ++m_revision;

        MeshGroup* group = record.m_group;
        const auto groupHandle = festd::find(group->m_instances, instance);
        FE_Assert(groupHandle != group->m_instances.end());
        group->m_instances.erase_unsorted(groupHandle);
        --group->m_instanceCount;

        m_meshInstanceTable->Free(record.m_tableRef);
        FreeHandle(instance);
        if (group->m_instanceCount == 0)
            DestroyGroup(group);
    }


    void MeshSceneModule::Update()
    {
        // Live groups still poll asset generations and streamed LOD residency.
        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            UpdateGroup(m_meshGroups[groupIndex]);
        });

        Bit::Traverse(m_boundsDirty.view(), [&](const uint32_t batchIndex) {
            UpdateBatchBounds(m_batches[batchIndex]);
        });
        m_boundsDirty.reset();

        Bit::Traverse(m_membershipDirty.view(), [&](const uint32_t batchIndex) {
            UpdateBatchMembership(m_batches[batchIndex]);
        });
        m_membershipDirty.reset();
    }


    void MeshSceneModule::UpdateBatchBounds(MeshBatch* batch)
    {
        Aabb bounds = batch->m_octreeEntry.m_bounds;
        for (const auto instanceRef : batch->m_meshInstances)
        {
            const auto instance = m_meshInstanceTable->ReadRow(instanceRef);
            const MeshGroup* group = m_meshGroups[instance.m_meshGroup.Get().m_rowIndex];
            const auto mesh = group->m_asset.Read();
            if (!mesh || mesh->m_submeshes.empty())
                continue;

            const Aabb local = mesh->m_submeshes[0].m_bounds;
            for (uint32_t corner = 0; corner < 8; ++corner)
            {
                const Vector4 world = Vector4(Math::Blend(local.min, local.max, corner), 1.0f) * instance.m_transform.Get();
                const Vector3 position(world.x, world.y, world.z);
                bounds = Math::Union(bounds, position);
            }
        }

        batch->m_octreeEntry.m_bounds = bounds;
        m_octree.InsertOrUpdate(batch->m_octreeEntry);
    }


    void MeshSceneModule::UpdateBatchMembership(MeshBatch* batch)
    {
        // Resize persistent membership only when instances are added, removed, or moved.
        const uint32_t count = batch->m_meshInstances.size();
        if (!m_memberTable->TryReallocateRows(batch->m_members, count))
        {
            m_memberTable->Free(batch->m_members);
            batch->m_members = m_memberTable->AllocateRows(count);
        }

        const auto row = m_batchTable->WriteRow(batch->m_tableRef);
        row.m_members.Get() = batch->m_members;
        row.m_drawTagMask.Get() = Vector2UInt(static_cast<uint32_t>(batch->m_drawTagMask.m_value),
                                              static_cast<uint32_t>(batch->m_drawTagMask.m_value >> 32));

        m_memberTable->CopyColumn(batch->m_members, festd::span(batch->m_meshInstances));
    }


    void MeshSceneModule::UpdateTransform(const MeshHandle instance, const Matrix4x4& transform)
    {
        const auto reference = TranslateHandle(instance);
        FE_Assert(reference.m_rowIndex != kInvalidIndex);
        m_meshInstanceTable->WriteRow(reference).m_transform.Get() = transform;
        m_boundsDirty.set(m_instances[instance.m_value].m_batch->m_tableRef.m_rowIndex);
    }


    void MeshSceneModule::MoveInstance(const MeshHandle instance, MeshBatch* batch)
    {
        FE_Assert(batch != nullptr && batch->m_parent == this);

        const auto reference = TranslateHandle(instance);
        FE_Assert(reference.m_rowIndex != kInvalidIndex);

        InstanceRecord& record = m_instances[instance.m_value];
        if (record.m_batch == batch)
            return;

        FE_Assert(batch->m_meshInstances.size() < MeshBatch::kMaxInstanceCount);
        MeshBatch* previous = record.m_batch;
        previous->m_meshInstances.erase_unsorted(
            festd::find_if(previous->m_meshInstances, [&](const DB::Ref<MeshInstanceTable> ref) {
                return ref.m_rowIndex == reference.m_rowIndex;
            }));

        previous->m_handles.erase_unsorted(festd::find(previous->m_handles, instance));
        m_membershipDirty.set(previous->m_tableRef.m_rowIndex);
        m_boundsDirty.set(previous->m_tableRef.m_rowIndex);
        batch->m_meshInstances.push_back(reference);
        batch->m_handles.push_back(instance);
        m_membershipDirty.set(batch->m_tableRef.m_rowIndex);
        m_boundsDirty.set(batch->m_tableRef.m_rowIndex);
        record.m_batch = batch;
    }


    void MeshSceneModule::UpdateMaterial(const MeshHandle handle, const IO::AssetLease<MaterialInstanceAsset>& materialLease)
    {
        FE_Assert(materialLease.IsValid() && materialLease.IsReady());

        const auto reference = TranslateHandle(handle);
        FE_Assert(reference.m_rowIndex != kInvalidIndex);

        InstanceRecord& record = m_instances[handle.m_value];
        MeshGroup* previous = record.m_group;
        MeshGroup* group = FindOrCreateMeshGroup(previous->m_asset, materialLease);
        if (group == previous)
            return;

        FreeInstanceParameters(previous->m_material, record.m_materialGeneration, record.m_parameters);

        const auto material = materialLease.Read();
        FE_Assert(material && material->m_runtime);
        record.m_parameters = material->m_runtime->AllocateInstanceParameters();
        record.m_materialGeneration = materialLease.GetGeneration();
        record.m_group = group;

        const auto instance = m_meshInstanceTable->WriteRow(reference);
        instance.m_meshGroup.Get() = group->m_tableRef;
        instance.m_instanceData.Get() = record.m_parameters.m_devicePointer;
        previous->m_instances.erase_unsorted(festd::find(previous->m_instances, handle));
        --previous->m_instanceCount;

        group->m_instances.push_back(handle);
        ++group->m_instanceCount;
        ++m_revision;

        if (previous->m_instanceCount == 0)
            DestroyGroup(previous);
    }


    void MeshSceneModule::SetBatchDrawTags(MeshBatch* batch, const DrawTagMask tags)
    {
        FE_Assert(batch != nullptr && batch->m_parent == this);
        batch->m_drawTagMask = tags;
        m_membershipDirty.set(batch->m_tableRef.m_rowIndex);
    }


    MeshPipelineRegistry& MeshSceneModule::UpdatePipelineRegistry(Core::FrameGraph& graph, Core::RingUploader& uploader,
                                                                  const Env::Name role)
    {
        MeshPipelineRegistry* registry = nullptr;
        for (auto& candidate : m_registries)
        {
            if (candidate.m_techniqueRole == role)
            {
                registry = &candidate;
                break;
            }
        }
        if (registry == nullptr)
        {
            registry = &m_registries.emplace_back();
            registry->m_techniqueRole = role;
        }
        if (registry->m_revision == m_revision)
            return *registry;

        // Retain bucket identities while refreshing capacities and group routing for this technique.
        for (auto& slot : registry->m_slots)
        {
            slot.m_instanceCapacity = 0;
            slot.m_workCapacity = 0;
        }

        festd::vector<MeshPass::PipelineRouting> routing(m_meshGroups.size());
        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            const MeshGroup* group = m_meshGroups[groupIndex];
            if (!group->m_buffer)
                return;

            const auto material = group->m_material.Read();
            if (!material || !material->m_runtime || !material->m_runtime->HasTechnique(role))
                return;

            const uint32_t meshletCount =
                m_meshLodInfoTable->ReadRow(group->m_lodsRef.m_rowIndex + group->m_residentLod).m_info.Get().m_meshletCount;
            if (meshletCount == 0)
                return;
            Core::GraphicsPipeline* pipeline = material->m_runtime->GetPipeline(role);
            if (pipeline == nullptr)
                return;

            uint32_t bucket = kInvalidIndex;
            for (uint32_t slotIndex = 0; slotIndex < registry->m_slots.size(); ++slotIndex)
            {
                if (registry->m_slots[slotIndex].m_pipeline.Get() == pipeline)
                {
                    bucket = slotIndex;
                    break;
                }
            }
            if (bucket == kInvalidIndex)
            {
                bucket = registry->m_slots.size();
                registry->m_slots.push_back({ pipeline, 0, 0 });
            }

            auto& slot = registry->m_slots[bucket];
            const uint32_t chunksPerInstance = Math::CeilDivide(meshletCount, MeshPass::kMeshletsPerWorkChunk);
            slot.m_instanceCapacity += group->m_instanceCount;
            slot.m_workCapacity += group->m_instanceCount * chunksPerInstance;
            routing[groupIndex].m_bucket = bucket;
        });

        if (!routing.empty())
        {
            registry->m_routing = Core::Buffer::CreateStructured<MeshPass::PipelineRouting>(graph.GetDevice(),
                                                                                            "MeshPipelineRouting",
                                                                                            routing.size());
            FE_Verify(uploader.UploadArray(graph, registry->m_routing.Get(), festd::span(routing)));
        }
        registry->m_revision = m_revision;
        return *registry;
    }


    void MeshSceneModule::UpdateRenderData(Core::FrameGraph& graph, Core::RingUploader&)
    {
        // Device-address geometry reads must be visible to the graph and retained through execution.
        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            const MeshGroup* group = m_meshGroups[groupIndex];
            if (!group->m_buffer)
                return;

            auto* desc = graph.AllocatePassData<Core::BufferAccessPassDesc>();
            desc->m_access = { group->m_buffer.Get(),
                               Core::BarrierSyncFlags::kAllShading,
                               Core::BarrierAccessFlags::kShaderRead };
            graph.AddPass("MeshGeometryReady", desc);
        });
    }


    void MeshSceneModule::PrepareRenderView(Core::FrameGraph& graph, Core::RingUploader& uploader)
    {
        auto& blackboard = graph.GetBlackboard();
        const auto& view = blackboard.Get<RendererViewData>();
        auto& culled = blackboard.Add<CulledMeshView>();
        const Matrix4x4 viewProjection = view.m_view->GetViewProjectionMatrix();
        RenderBatchCollector visibility(graph.GetAllocator(), viewProjection);
        festd::pmr::vector<MeshPass::CullDispatch> accepted{ graph.GetAllocator() };
        const auto collectBatch = [&](const MeshBatch* batch) {
            if (batch == nullptr)
                return;

            if (!visibility.IsVisible(batch->m_octreeEntry.m_bounds))
                return;

            for (uint32_t firstMember = 0; firstMember < batch->m_members.m_count;
                 firstMember += MeshPass::kInstancesPerCullGroup)
            {
                accepted.push_back({ batch->m_tableRef.m_rowIndex, firstMember });
            }
        };

        if (!Math::Overlaps(visibility.GetFrustumBounds(), m_octree.GetBounds()))
        {
            for (const MeshBatch* batch : m_batches)
                collectBatch(batch);
        }
        else
        {
            m_octree.Traverse(visibility.GetFrustumBounds(), [&](const Aabb&, const festd::span<OctreeEntry*> entries) {
                for (const OctreeEntry* entry : entries)
                    collectBatch(m_batches[entry->m_userIndex]);
            });
        }

        MeshPass::ViewData data{};
        data.m_viewProjection = viewProjection;
        data.m_instances = m_meshInstanceTable->GetDeviceAddress();
        data.m_groups = m_meshGroupTable->GetDeviceAddress();
        data.m_lods = m_meshLodInfoTable->GetDeviceAddress();
        data.m_materials = m_materialInstanceTable->GetDeviceAddress();
        data.m_batches = m_batchTable->GetDeviceAddress();
        data.m_members = m_memberTable->GetDeviceAddress();
        GpuMeshWorkBuilder::CullView(graph, uploader, accepted, data, culled);
    }


    void MeshSceneModule::AddRenderPasses(Core::FrameGraph& graph, Core::RingUploader& uploader, const SceneRenderPass& pass)
    {
        const auto& culled = graph.GetBlackboard().Get<CulledMeshView>();
        if (!culled.m_classification)
            return;

        MeshPipelineRegistry& registry = UpdatePipelineRegistry(graph, uploader, pass.m_techniqueRole);
        PreparedMeshPass prepared;
        const DrawTagMask mask(pass.m_drawTag);
        const Vector2UInt drawTagMask(static_cast<uint32_t>(mask.m_value), static_cast<uint32_t>(mask.m_value >> 32));
        GpuMeshWorkBuilder::Build(graph, uploader, registry, culled, drawTagMask, prepared);
        if (!prepared.m_view)
            return;

        auto* desc = graph.AllocatePassData<MeshPass::PassDesc>();
        constexpr auto sync = Core::BarrierSyncFlags::kAmplificationShading | Core::BarrierSyncFlags::kMeshShading
            | Core::BarrierSyncFlags::kPixelShading;
        desc->m_view = { prepared.m_view.Get(), sync, Core::BarrierAccessFlags::kShaderRead };
        desc->m_visibleInstances = { prepared.m_instances.Get(), sync, Core::BarrierAccessFlags::kShaderRead };
        desc->m_workChunks = { prepared.m_work.Get(), sync, Core::BarrierAccessFlags::kShaderRead };
        desc->m_commands = { prepared.m_commands.Get(), sync, Core::BarrierAccessFlags::kShaderRead };
        desc->m_arguments = Core::BufferView(prepared.m_arguments.Get());

        const BufferSRVDescriptor viewAddress = graph.GetSRV(prepared.m_view.Get());
        graph.AddPass(festd::string_view(pass.m_techniqueRole.c_str(), pass.m_techniqueRole.size()),
                      desc,
                      pass.m_passDescToken,
                      [submissions = std::move(prepared.m_submissions), viewAddress](Core::FrameGraphContext& context) {
                          context.BeginRenderPass();
                          for (const auto& submission : submissions)
                          {
                              for (uint32_t page = 0; page < submission.m_commandCount; ++page)
                              {
                                  context.SetPipeline(submission.m_pipeline.Get());
                                  MeshPass::Constants constants{ viewAddress, submission.m_firstCommand + page };
                                  context.PushConstants(constants);
                                  context.DispatchMeshIndirect(constants.m_commandIndex
                                                               * sizeof(MeshPass::MeshDispatchArguments));
                              }
                          }
                          context.EndRenderPass();
                      });
    }


    void MeshSceneModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    MeshHandle MeshSceneModule::AllocateHandle(const DB::Ref<MeshInstanceTable> sourceIndex, MeshBatch* batch, MeshGroup* group,
                                               const MaterialParameterAllocator::Allocation parameters)
    {
        EnsureCapacity();

        const uint32_t handleIndex = m_freeHandles.find_first();
        FE_Assert(handleIndex != kInvalidIndex);
        m_freeHandles.reset(handleIndex);

        InstanceRecord& record = m_instances[handleIndex];
        record.m_tableRef = sourceIndex;
        record.m_batch = batch;
        record.m_group = group;
        record.m_parameters = parameters;
        record.m_materialGeneration = group->m_materialGeneration;

        MeshHandle handle;
        handle.m_value = handleIndex;
        handle.m_generation = record.m_generation;
        return handle;
    }


    void MeshSceneModule::FreeHandle(const MeshHandle handle)
    {
        InstanceRecord& record = m_instances[handle.m_value];
        record.m_tableRef.Invalidate();
        record.m_batch = nullptr;
        record.m_group = nullptr;
        record.m_parameters = {};
        record.m_materialGeneration = 0;
        ++record.m_generation;
        m_freeHandles.set(handle.m_value);
    }


    void MeshSceneModule::EnsureCapacity()
    {
        const uint32_t capacity = m_meshInstanceTable->GetReservedRowCount();
        if (m_freeHandles.size() < capacity)
        {
            m_freeHandles.resize(capacity, true);
            m_instances.resize(capacity);
        }
    }


    MeshGroup* MeshSceneModule::FindOrCreateMeshGroup(const IO::AssetLease<MeshAsset>& meshAsset,
                                                      const IO::AssetLease<MaterialInstanceAsset>& material)
    {
        for (MeshGroup* group : m_meshGroups)
        {
            if (group != nullptr && group->m_asset.GetAssetSlot() == meshAsset.GetAssetSlot()
                && group->m_material.GetAssetSlot() == material.GetAssetSlot())
            {
                return group;
            }
        }

        auto* meshGroup = Memory::DefaultNew<MeshGroup>();
        meshGroup->m_asset = meshAsset;
        meshGroup->m_material = material;
        meshGroup->m_tableRef = m_meshGroupTable->AllocateRow();
        meshGroup->m_materialRef = m_materialInstanceTable->AllocateRow();
        m_meshGroups.resize(m_meshGroupTable->GetReservedRowCount());
        m_meshGroups[meshGroup->m_tableRef.m_rowIndex] = meshGroup;
        m_activeMeshGroups.resize(m_meshGroups.size(), false);
        m_activeMeshGroups.set(meshGroup->m_tableRef.m_rowIndex);
        UpdateGroup(meshGroup);
        return meshGroup;
    }


    void MeshSceneModule::UpdateGroup(MeshGroup* group)
    {
        const IO::AssetRead<MeshAsset> mesh = group->m_asset.Read();
        const IO::AssetRead<MaterialInstanceAsset> material = group->m_material.Read();
        if (!mesh || !material || !material->m_runtime || !mesh->m_buffer)
        {
            if (m_meshGroupTable->ReadRow(group->m_tableRef).m_renderable.Get() != 0)
            {
                m_meshGroupTable->WriteRow(group->m_tableRef).m_renderable.Get() = 0;
                ++m_revision;
            }
            if (group->m_buffer)
            {
                group->m_buffer.Reset();
                group->m_meshGeneration = 0;
                ++m_revision;
            }
            return;
        }

        const uint32_t meshGeneration = group->m_asset.GetGeneration();
        const uint32_t materialGeneration = group->m_material.GetGeneration();
        const bool meshChanged = group->m_meshGeneration != meshGeneration || group->m_buffer.Get() != mesh->m_buffer.Get()
            || group->m_residentLod != mesh->m_residentLod;
        const bool materialChanged = group->m_materialGeneration != materialGeneration;
        if (!meshChanged && !materialChanged)
            return;

        ++m_revision;
        const MeshGroupTable::RWRow tableRow = m_meshGroupTable->WriteRow(group->m_tableRef);
        if (meshChanged)
        {
            FE_Assert(!mesh->m_submeshes.empty());
            const MeshSubmeshAssetInfo& submesh = mesh->m_submeshes[0];
            tableRow.m_bounds.Get().m_min = PackedVector3F(submesh.m_bounds.min);
            tableRow.m_bounds.Get().m_max = PackedVector3F(submesh.m_bounds.max);
            for (const MeshHandle handle : group->m_instances)
                m_boundsDirty.set(m_instances[handle.m_value].m_batch->m_tableRef.m_rowIndex);

            FE_Assert(!submesh.m_lods.empty());
            if (group->m_lodsRef.m_count != submesh.m_lods.size())
            {
                if (group->m_lodsRef.m_count != 0)
                    m_meshLodInfoTable->Free(group->m_lodsRef);
                group->m_lodsRef = m_meshLodInfoTable->AllocateRows(submesh.m_lods.size());
            }

            Memory::FiberTempAllocator temp;
            festd::pmr::inline_vector<Core::MeshLodInfo> lodInfos{ &temp };
            lodInfos.reserve(submesh.m_lods.size());
            for (const MeshLodAssetInfo& assetLod : submesh.m_lods)
            {
                Core::MeshLodInfo& lod = lodInfos.emplace_back();
                lod.m_vertexCount = assetLod.m_vertexCount;
                lod.m_indexCount = assetLod.m_indexCount;
                lod.m_meshletCount = assetLod.m_meshletCount;
                lod.m_primitiveCount = assetLod.m_primitiveCount;
            }
            m_meshLodInfoTable->CopyColumn(group->m_lodsRef, lodInfos);

            const uint32_t lodIndex = mesh->m_residentLod;
            FE_Assert(lodIndex < submesh.m_lods.size());
            uint64_t geometryOffset = 0;
            for (uint32_t previousLod = 0; previousLod < lodIndex; ++previousLod)
            {
                for (const MeshSubmeshAssetInfo& previousSubmesh : mesh->m_submeshes)
                {
                    FE_Assert(previousLod < previousSubmesh.m_lods.size());
                    const MeshLodAssetInfo& info = previousSubmesh.m_lods[previousLod];
                    geometryOffset += static_cast<uint64_t>(info.m_vertexCount) * mesh->m_vertexStride;
                    geometryOffset += static_cast<uint64_t>(info.m_indexCount) * sizeof(uint32_t);
                    geometryOffset += static_cast<uint64_t>(info.m_meshletCount) * sizeof(Core::MeshletHeader);
                    geometryOffset += static_cast<uint64_t>(info.m_primitiveCount) * sizeof(Core::PackedTriangle);
                    geometryOffset += static_cast<uint64_t>(info.m_meshletCount) * sizeof(PackedVector4F);
                }
            }
            FE_Assert(geometryOffset < mesh->m_buffer->GetDesc().m_size);

            tableRow.m_geometry.Get() = BufferPointer{ mesh->m_buffer->GetDeviceAddress() + geometryOffset };
            tableRow.m_lods.Get() = DB::Slice<MeshLodInfoTable>{ group->m_lodsRef.m_rowIndex + lodIndex, 1 };
            group->m_buffer = mesh->m_buffer;
            group->m_meshGeneration = meshGeneration;
            group->m_residentLod = mesh->m_residentLod;
        }

        if (materialChanged)
        {
            m_materialInstanceTable->WriteRow(group->m_materialRef).m_materialParameters.Get() =
                material->m_runtime->GetMaterialParameters();
            tableRow.m_materialInstance.Get() = group->m_materialRef;
            for (const MeshHandle handle : group->m_instances)
            {
                InstanceRecord& record = m_instances[handle.m_value];
                record.m_parameters = material->m_runtime->AllocateInstanceParameters();
                record.m_materialGeneration = materialGeneration;
                m_meshInstanceTable->WriteRow(record.m_tableRef).m_instanceData.Get() = record.m_parameters.m_devicePointer;
            }

            group->m_materialGeneration = materialGeneration;
        }
        tableRow.m_renderable.Get() = 1;
    }


    void MeshSceneModule::DestroyGroup(MeshGroup* group)
    {
        FE_Assert(group->m_instanceCount == 0);
        ++m_revision;
        m_meshGroups[group->m_tableRef.m_rowIndex] = nullptr;
        m_activeMeshGroups.reset(group->m_tableRef.m_rowIndex);
        if (group->m_lodsRef.m_count != 0)
            m_meshLodInfoTable->Free(group->m_lodsRef);

        m_materialInstanceTable->Free(group->m_materialRef);
        m_meshGroupTable->Free(group->m_tableRef);
        Memory::DefaultDelete(group);
    }


    IO::AssetRead<MeshAsset> MeshSceneModule::FindAsset(const DB::Ref<MeshGroupTable> group) const
    {
        if (group.m_rowIndex >= m_meshGroups.size() || m_meshGroups[group.m_rowIndex] == nullptr)
            return {};

        return m_meshGroups[group.m_rowIndex]->m_asset.Read();
    }


    IO::AssetRead<MaterialInstanceAsset> MeshSceneModule::FindMaterial(const DB::Ref<MeshGroupTable> group) const
    {
        if (group.m_rowIndex >= m_meshGroups.size() || m_meshGroups[group.m_rowIndex] == nullptr)
            return {};

        return m_meshGroups[group.m_rowIndex]->m_material.Read();
    }


    DB::Ref<MeshInstanceTable> MeshSceneModule::TranslateHandle(const MeshHandle handle) const
    {
        if (handle.IsValid() && handle.m_value < m_instances.size()
            && m_instances[handle.m_value].m_generation == handle.m_generation)
        {
            return m_instances[handle.m_value].m_tableRef;
        }

        return DB::Ref<MeshInstanceTable>::CreateInvalid();
    }
} // namespace FE::Graphics
