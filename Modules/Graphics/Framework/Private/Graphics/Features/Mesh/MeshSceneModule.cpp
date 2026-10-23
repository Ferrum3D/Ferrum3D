#include <Core/Jobs/Jobs.h>
#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>
#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Scene/SurfaceEvaluation.h>
#include <Graphics/Tables/MaterialInstanceTable.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

#include <Shaders/Passes/MeshPass/IndirectArguments.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>
#include <Shaders/Passes/Visibility/Contracts.h>

namespace FE::Graphics
{
    namespace
    {
        template<class TFunction>
        void RunMeshJobs(const uint32_t count, const TFunction& function, const uint32_t grainSize = 32)
        {
            FE_PROFILER_ZONE();

            const uint32_t jobCount = Math::Min(8u, Math::CeilDivide(count, grainSize));
            if (jobCount <= 1)
            {
                for (uint32_t i = 0; i < count; ++i)
                    function(i);

                return;
            }

            const uint32_t batchSize = Math::CeilDivide(count, jobCount);
            const Rc<WaitGroup> completion = WaitGroup::Create();
            for (uint32_t job = 1; job < jobCount; ++job)
            {
                completion->Add(1);
                Jobs::DispatchForeground(
                    [&, job] {
                        FE_PROFILER_ZONE_NAMED("MeshWorkRange");

                        const uint32_t end = Math::Min(count, (job + 1) * batchSize);
                        ZoneTextF("Items %u..%u", job * batchSize, end);
                        for (uint32_t i = job * batchSize; i < end; ++i)
                            function(i);
                    },
                    completion.Get());
            }

            for (uint32_t i = 0; i < batchSize; ++i)
                function(i);

            completion->Signal();
            completion->Wait();
        }


        FE_COMPUTE_PIPELINE_SET(CountMeshSurfacePipeline, "Shaders/Passes/MeshPass/CountMeshSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterMeshSurfacePipeline, "Shaders/Passes/MeshPass/ScatterMeshSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(MeshVisibilityDebugPipeline, "Shaders/Passes/MeshPass/VisibilityDebug.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(MeshSurfaceDebugPipeline, "Shaders/Passes/MeshPass/MeshSurfaceDebug.cs.hlsl");

        static_assert(MeshPass::kMaxLodCount == Visibility::kMaxLodCount);
        static_assert(MeshPass::kMaxLodCount == Culling::Stats::kMaxLodCount);
        static_assert(festd::to_underlying(SceneRenderPhase::kSingle) == MeshPass::CullPhase::kSingle);
        static_assert(festd::to_underlying(SceneRenderPhase::kEarly) == MeshPass::CullPhase::kEarly);
        static_assert(festd::to_underlying(SceneRenderPhase::kLate) == MeshPass::CullPhase::kLate);

        // Views that stop rendering keep their culling state for a few frames, e.g. while a window is minimized.
        constexpr uint64_t kViewCullingStateLifetime = 64;

        // Instances that do not fit render without meshlet visibility history: all their meshlets are tested in the late phase.
        constexpr uint32_t kMeshletVisibilityArenaWords = 1u << 22;


        Rc<Core::Buffer> CreateZeroedViewBuffer(Core::FrameGraph& graph, Core::RingUploader& uploader, const Env::Name name,
                                                const uint32_t wordCount)
        {
            Rc<Core::Buffer> buffer = Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), name, wordCount);

            Core::ResourceCommitParams commitParams;
            commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
            commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite
                | Core::BarrierAccessFlags::kCopyDest;
            graph.GetResourcePool()->CommitBufferMemory(buffer.Get(), commitParams);
            FE_Verify(uploader.UploadGenerated(graph, buffer.Get(), wordCount * sizeof(uint32_t), [wordCount](void* target) {
                memset(target, 0, wordCount * sizeof(uint32_t));
            }));

            return buffer;
        }


        uint32_t CalculateLodPayloadSize(const MeshAsset& mesh, const uint32_t assetLodIndex)
        {
            uint32_t size = 0;
            for (const MeshSubmeshAssetInfo& submesh : mesh.m_submeshes)
            {
                FE_Assert(assetLodIndex < submesh.m_lods.size());
                const MeshLodAssetInfo& info = submesh.m_lods[assetLodIndex];
                size += info.m_vertexCount * mesh.m_vertexStride;
                size += info.m_indexCount * sizeof(uint32_t);
                size += info.m_meshletCount * sizeof(Core::MeshletHeader);
                size += info.m_primitiveCount * sizeof(Core::PackedTriangle);
                size += info.m_meshletCount * sizeof(PackedVector4F);
            }

            return size;
        }


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


        MeshPass::MeshCullData CreateCullData(const Culling::PassData& culling, const float viewportHeight)
        {
            // An LOD is acceptable when error * scale / distance * projection * height / 2 <= threshold.
            const float threshold = Math::Max(culling.m_settings.m_lodErrorThreshold, 1e-3f);
            const float lodScale = culling.m_projectionScaleY * viewportHeight * 0.5f / threshold;

            MeshPass::MeshCullData cull{};
            cull.m_viewProjection = culling.m_viewProjection;
            cull.m_occlusionViewProjection = culling.m_occlusionViewProjection;
            cull.m_cameraPosition = PackedVector4F(Vector4(culling.m_cameraPosition, lodScale));
            cull.m_flags = CalculateCullFlags(culling.m_settings);
            return cull;
        }


        Culling::Stats ConvertCullingStats(const uint32_t (&values)[MeshPass::CullStat::kCount])
        {
            Culling::Stats stats;
            stats.m_instanceCandidates = values[MeshPass::CullStat::kInstanceCandidates];
            stats.m_instanceFrustumCulled = values[MeshPass::CullStat::kInstanceFrustumCulled];
            stats.m_instanceOcclusionCulled = values[MeshPass::CullStat::kInstanceOcclusionCulled];
            stats.m_instancesRendered = values[MeshPass::CullStat::kInstanceRendered];
            stats.m_instancesRenderedLate = values[MeshPass::CullStat::kInstanceRenderedLate];
            stats.m_meshletCandidates = values[MeshPass::CullStat::kMeshletCandidates];
            stats.m_meshletFrustumCulled = values[MeshPass::CullStat::kMeshletFrustumCulled];
            stats.m_meshletOcclusionCulled = values[MeshPass::CullStat::kMeshletOcclusionCulled];
            stats.m_meshletsRendered = values[MeshPass::CullStat::kMeshletRendered];
            stats.m_trianglesRendered = values[MeshPass::CullStat::kTriangleRendered];
            for (uint32_t lodIndex = 0; lodIndex < MeshPass::kMaxLodCount; ++lodIndex)
                stats.m_lodInstances[lodIndex] = values[MeshPass::CullStat::kLodHistogram + lodIndex];

            stats.m_isValid = true;
            return stats;
        }
    } // namespace


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
        m_visibilityType = scene->GetRenderData().RegisterVisibilityType();
        const auto compute = renderer->GetGraphicsQueue()->GetDevice()->GetComputeCapabilities();
        m_evaluationWaveLaneCount = compute.m_waveLaneCount == 64 ? 64 : 32;

        m_meshLodInfoTable = Memory::DefaultNew<MeshLodInfoTable>(database);
        m_meshGroupTable = Memory::DefaultNew<MeshGroupTable>(database);
        m_meshInstanceTable = Memory::DefaultNew<MeshInstanceTable>(database);
        m_materialInstanceTable = Memory::DefaultNew<MaterialInstanceTable>(database);
        m_batchTable = Memory::DefaultNew<MeshBatchTable>(database);
        m_memberTable = Memory::DefaultNew<MeshMemberTable>(database);
        m_meshletVisibilityAllocator.Setup(kMeshletVisibilityArenaWords);
    }


    MeshSceneModule::~MeshSceneModule()
    {
        m_scene->GetRenderData().ReleaseVisibilityType(m_visibilityType);
        for (MeshBatch* batch : m_batches)
        {
            if (batch != nullptr)
                DestroyBatch(batch);
        }

        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            DestroyGroup(m_meshGroups[groupIndex]);
        });

        m_meshletVisibilityAllocator.Shutdown();
    }


    MeshBatch* MeshSceneModule::CreateBatch(const MeshBatchDesc& desc)
    {
        FE_PROFILER_ZONE();

        FE_Assert(desc.m_bounds.IsValid());
        auto* batch = Memory::DefaultNew<MeshBatch>();
        batch->m_parent = this;
        batch->m_tableRef = m_batchTable->AllocateRow();
        batch->m_drawTagMask = desc.m_drawTagMask;
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
        m_octree.InsertOrUpdate(batch->m_octreeEntry, desc.m_bounds);
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
        FE_PROFILER_ZONE();

        FE_Assert(desc.m_asset.IsValid() && desc.m_asset.IsReady());
        FE_Assert(desc.m_material.IsValid() && desc.m_material.IsReady());
        FE_Assert(desc.m_batch != nullptr && desc.m_batch->m_parent == this);
        FE_Assert(desc.m_batch->m_meshInstances.size() < MeshBatch::kMaxInstanceCount);

        MeshGroup* meshGroup = FindOrCreateMeshGroup(desc.m_asset, desc.m_material);
        const IO::AssetRead<MaterialInstanceAsset> material = desc.m_material.Read();
        FE_Assert(material && material->m_runtime);
        const MaterialParameterAllocator::Allocation parameters = material->m_runtime->AllocateInstanceParameters();

        const DB::Ref<MeshInstanceTable> instanceRef = m_meshInstanceTable->AllocateRow();
        FE_Assert(instanceRef.m_rowIndex <= Visibility::kMaxInstanceRow);
        const MeshHandle handle = AllocateHandle(instanceRef, desc.m_batch, meshGroup, parameters);
        AllocateMeshletVisibility(m_instances[handle.m_value]);

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
        FE_PROFILER_ZONE();

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

        FreeMeshletVisibility(m_instances[instance.m_value]);
        m_meshInstanceTable->Free(record.m_tableRef);
        FreeHandle(instance);
        if (group->m_instanceCount == 0)
            DestroyGroup(group);
    }


    void MeshSceneModule::Update()
    {
        FE_PROFILER_ZONE();

        ++m_frameIndex;
        ReleaseUnusedViewCullingStates();

        // Live groups still poll asset generations and streamed LOD residency.
        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            UpdateGroup(m_meshGroups[groupIndex]);
        });

        // Workers own distinct rows. Publish shared database/batch dirty bits only after extraction joins.
        for (InstanceRecord& instance : m_instances)
        {
            if (!instance.m_transformDirty)
                continue;

            FE_Assert(instance.m_tableRef.m_rowIndex != kInvalidIndex);
            (void)m_meshInstanceTable->WriteRow(instance.m_tableRef);
            m_boundsDirty.set(instance.m_batch->m_tableRef.m_rowIndex);
            instance.m_transformDirty = false;
        }

        m_dirtyBatches.clear();
        Bit::Traverse(m_boundsDirty.view(), [&](const uint32_t batchIndex) {
            m_dirtyBatches.push_back(batchIndex);
        });
        m_dirtyBatchBounds.resize(m_dirtyBatches.size());
        RunMeshJobs(m_dirtyBatches.size(), [&](const uint32_t index) {
            m_dirtyBatchBounds[index] = CalculateBatchBounds(m_batches[m_dirtyBatches[index]]);
        });
        for (uint32_t i = 0; i < m_dirtyBatches.size(); ++i)
            m_octree.InsertOrUpdate(m_batches[m_dirtyBatches[i]]->m_octreeEntry, m_dirtyBatchBounds[i]);

        m_boundsDirty.reset();

        Bit::Traverse(m_membershipDirty.view(), [&](const uint32_t batchIndex) {
            UpdateBatchMembership(m_batches[batchIndex]);
        });
        m_membershipDirty.reset();
    }


    Aabb MeshSceneModule::CalculateBatchBounds(const MeshBatch* batch) const
    {
        Aabb bounds = Aabb::kInvalid;
        for (const MeshHandle handle : batch->m_handles)
        {
            const InstanceRecord& instance = m_instances[handle.m_value];
            const Aabb& localBounds = instance.m_group->m_localBounds;
            if (!localBounds.IsValid())
                continue;

            for (uint32_t corner = 0; corner < 8; ++corner)
            {
                const Vector3 localPosition = Math::Blend(localBounds.min, localBounds.max, corner);
                const Vector4 worldPosition = Vector4(localPosition, 1.0f) * *instance.m_transform;
                bounds = Math::Union(bounds, Vector3(worldPosition.x, worldPosition.y, worldPosition.z));
            }
        }

        // Empty batches still have a valid index entry; no dispatch is emitted for zero members.
        return bounds.IsValid() ? bounds : Aabb::kZero;
    }


    void MeshSceneModule::UpdateBatchMembership(MeshBatch* batch)
    {
        FE_PROFILER_ZONE();

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
        InstanceRecord& record = m_instances[instance.m_value];
        *record.m_transform = transform;
        record.m_transformDirty = true;
    }


    void MeshSceneModule::MoveInstance(const MeshHandle instance, MeshBatch* batch)
    {
        FE_PROFILER_ZONE();

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
        FE_PROFILER_ZONE();

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
        AllocateMeshletVisibility(record);

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
        FE_PROFILER_ZONE();

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

        registry->m_slots.clear();

        festd::vector<MeshPass::PipelineRouting> routing(m_meshGroups.size());
        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            const MeshGroup* group = m_meshGroups[groupIndex];
            if (!group->m_buffer)
                return;

            const auto material = group->m_material.Read();
            if (!material || !material->m_runtime || !material->m_runtime->HasTechnique(role))
                return;

            // LOD selection may choose any resident LOD, so capacities cover the largest one.
            const uint32_t meshletCount = group->m_maxMeshletCount;
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

        registry->m_submissions.clear();
        registry->m_instanceCapacity = 0;
        registry->m_workCapacity = 0;
        registry->m_commandCapacity = 0;

        festd::inline_vector<MeshPass::BucketLayout, 16> layouts;
        for (const MeshPipelineSlot& slot : registry->m_slots)
        {
            const uint32_t meshletCapacity = slot.m_workCapacity * MeshPass::kMeshletsPerWorkChunk;
            const uint32_t pageCount = Math::CeilDivide(meshletCapacity, MeshPass::kMeshletsPerCommand);
            layouts.push_back({ registry->m_commandCapacity, pageCount });
            if (pageCount != 0)
                registry->m_submissions.push_back({ slot.m_pipeline, registry->m_commandCapacity, pageCount });

            registry->m_instanceCapacity += slot.m_instanceCapacity;
            registry->m_workCapacity += slot.m_workCapacity;
            registry->m_commandCapacity += pageCount;
        }

        if (!layouts.empty())
        {
            registry->m_layouts =
                Core::Buffer::CreateStructured<MeshPass::BucketLayout>(graph.GetDevice(), "MeshBucketLayouts", layouts.size());
            FE_Verify(uploader.UploadArray(graph, registry->m_layouts.Get(), festd::span(layouts)));
        }
        else
        {
            registry->m_layouts.Reset();
            registry->m_routing.Reset();
        }

        registry->m_revision = m_revision;
        return *registry;
    }


    void MeshSceneModule::UpdateRenderData(Core::FrameGraph& graph, Core::RingUploader& uploader)
    {
        FE_PROFILER_ZONE();

        MeshPass::MeshSceneData sceneData{};
        sceneData.m_instances = m_meshInstanceTable->GetDeviceAddress();
        sceneData.m_groups = m_meshGroupTable->GetDeviceAddress();
        sceneData.m_lods = m_meshLodInfoTable->GetDeviceAddress();
        sceneData.m_materials = m_materialInstanceTable->GetDeviceAddress();
        sceneData.m_batches = m_batchTable->GetDeviceAddress();
        sceneData.m_members = m_memberTable->GetDeviceAddress();
        sceneData.m_visibilityType = m_visibilityType;

        const festd::array<uint64_t, 6> tableAddresses{
            sceneData.m_instances.GetDeviceAddress(), sceneData.m_groups.GetDeviceAddress(),
            sceneData.m_lods.GetDeviceAddress(),      sceneData.m_materials.GetDeviceAddress(),
            sceneData.m_batches.GetDeviceAddress(),   sceneData.m_members.GetDeviceAddress()
        };
        if (m_sceneData == nullptr || tableAddresses != m_sceneTableAddresses)
        {
            // Immutable versions preserve constants already referenced by submitted frames.
            m_sceneData = Core::Buffer::CreateStructured<MeshPass::MeshSceneData>(graph.GetDevice(), "MeshScene", 1);
            FE_Verify(uploader.Upload(graph, m_sceneData.Get(), sceneData));
            m_sceneTableAddresses = tableAddresses;
        }

        const auto& pipelines = m_scene->GetRenderData().m_evaluationPipelines;
        const uint32_t groupRowCount = m_meshGroupTable->GetReservedRowCount();
        const bool routesChanged = m_evaluationRoutingRevision != m_revision
            || m_evaluationPipelineRevision != pipelines.GetRevision() || m_evaluationGroupRowCount != groupRowCount;
        if (routesChanged)
        {
            festd::inline_vector<Visibility::SurfaceBinding, 16> bindings;
            for (const MeshGroup* group : m_meshGroups)
            {
                if (group == nullptr || group->m_buffer == nullptr || group->m_instanceCount == 0)
                    continue;

                const auto material = m_materialInstanceTable->ReadRow(group->m_materialRef);
                if (material.m_evaluationSlot.Get() != kInvalidIndex && material.m_shadingSlot.Get() < 254)
                    bindings.push_back({ group->m_tableRef.m_rowIndex, material.m_evaluationSlot.Get() });
            }

            m_evaluationRouting =
                Visibility::CreateSurfaceRouting(graph.GetDevice(), bindings, groupRowCount, pipelines.GetSlots().size());
            m_evaluationRoutingRevision = m_revision;
            m_evaluationPipelineRevision = pipelines.GetRevision();
            m_evaluationGroupRowCount = groupRowCount;
        }
    }


    void MeshSceneModule::AddGeometryDependencies(Core::FrameGraph& graph, Core::Buffer* buffer) const
    {
        // Geometry is read by rasterization and reconstruction, independently of view culling.
        Bit::Traverse(m_activeMeshGroups.view(), [&](const uint32_t groupIndex) {
            const MeshGroup* group = m_meshGroups[groupIndex];
            if (group->m_buffer != nullptr)
                graph.AddBufferDependency(buffer, group->m_buffer.Get());
        });
    }


    void MeshSceneModule::CollectCullDispatches(const Culling::PassData& culling)
    {
        FE_PROFILER_ZONE();

        for (auto& dispatches : m_cullDispatches)
            dispatches.clear();

        const auto addBatch = [](festd::vector<MeshPass::CullDispatch>& dispatches, const MeshBatch& batch) {
            for (uint32_t first = 0; first < batch.m_members.m_count; first += MeshPass::kInstancesPerCullGroup)
                dispatches.push_back({ batch.m_tableRef.m_rowIndex, first });
        };

        // Without frustum culling every batch is a candidate, including those outside the camera.
        if (!culling.m_settings.m_instanceFrustumCulling)
        {
            for (const MeshBatch* batch : m_batches)
            {
                if (batch != nullptr)
                    addBatch(m_cullDispatches[0], *batch);
            }

            return;
        }

        const Frustum frustum(culling.m_viewProjection);
        m_octree.TraverseParallel(frustum, [&](const uint32_t taskIndex, const festd::span<OctreeEntry* const> entries) {
            for (const OctreeEntry* entry : entries)
                addBatch(m_cullDispatches[taskIndex], *m_batches[entry->m_userIndex]);
        });
    }


    void MeshSceneModule::PrepareRenderView(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                                            Core::RingUploader& uploader)
    {
        FE_PROFILER_ZONE_NAMED("BuildCpuMeshWork");

        const auto& view = blackboard.Get<RendererViewData>();
        const Culling::PassData& culling = Culling::GetPassData(blackboard);
        auto& culled = blackboard.Add<CulledMeshView>();

        ViewCullingState& state = AcquireViewCullingState(graph, uploader, view.m_view);
        blackboard.Add<ViewCullingState*>(&state);
        ResolveCullingStats(state, culling.m_stats);

        CollectCullDispatches(culling);

        uint32_t offsets[Octree::kMaxTraversalTasks];
        uint32_t count = 0;
        for (uint32_t i = 0; i < Octree::kMaxTraversalTasks; ++i)
        {
            offsets[i] = count;
            count += m_cullDispatches[i].size();
        }

        MeshPass::MeshViewData data{};
        data.m_view = blackboard.Get<ViewData>();
        data.m_cull = CreateCullData(culling, view.m_viewportRect.Height());
        data.m_scene = graph.GetSRV(m_sceneData.Get());

        // A frozen pyramid replaces the two phases with a single one, matching Visibility::AddPasses.
        const bool isOcclusionCulled = culling.m_settings.IsOcclusionCullingEnabled();
        const Tools::DepthPyramid::Output* frozenDepthPyramid = culling.GetFrozenDepthPyramid();
        const bool isTwoPhase = isOcclusionCulled && frozenDepthPyramid == nullptr;
        const uint32_t phase = isTwoPhase ? MeshPass::CullPhase::kEarly : MeshPass::CullPhase::kSingle;

        MeshCullViewResources resources;
        resources.m_visibility = state.m_visibility.Get();
        resources.m_visibilityCapacity = state.m_visibilityCapacity;
        resources.m_meshletVisibility = state.m_meshletVisibility.Get();
        resources.m_meshletVisibilityCapacity = state.m_meshletVisibilityCapacity;
        resources.m_stats = state.m_stats.Get();
        resources.m_depthPyramid = isOcclusionCulled ? frozenDepthPyramid : nullptr;
        GpuMeshWorkBuilder::CullView(
            graph,
            uploader,
            count,
            [&](const festd::span<MeshPass::CullDispatch> target) {
                // Prefix offsets give workers disjoint mapped ranges; only the render fiber touches the uploader/graph.
                RunMeshJobs(
                    Octree::kMaxTraversalTasks,
                    [&](const uint32_t i) {
                        if (!m_cullDispatches[i].empty())
                            festd::copy(m_cullDispatches[i], target.data() + offsets[i]);
                    },
                    count >= 1024 ? 8 : Octree::kMaxTraversalTasks);
            },
            data,
            phase,
            resources,
            culled);

        // Meshlet culling reads bounding spheres from the geometry of every group.
        if (culled.m_view)
            AddGeometryDependencies(graph, culled.m_view.Get());

        SurfaceEvaluationBatch batch;
        batch.m_objectData = Core::Buffer::CreateStructured<MeshPass::MeshViewData>(graph.GetDevice(), "MeshSurfaceView", 1);
        graph.AddBufferDependency(batch.m_objectData.Get(), m_sceneData.Get());
        AddGeometryDependencies(graph, batch.m_objectData.Get());
        FE_Verify(uploader.Upload(graph, batch.m_objectData.Get(), data));

        batch.m_countPipeline = CountMeshSurfacePipeline::GetPipeline();
        batch.m_scatterPipeline = ScatterMeshSurfacePipeline::GetPipeline();
        batch.m_visibilityDebugPipeline = MeshVisibilityDebugPipeline::GetPipeline();
        batch.m_surfaceDebugPipeline = MeshSurfaceDebugPipeline::GetPipeline();
        batch.m_routing = &m_evaluationRouting;
        batch.m_waveLaneCount = m_evaluationWaveLaneCount;

        blackboard.Get<SurfaceEvaluationInputs>().push_back(std::move(batch));
    }


    void MeshSceneModule::AddRenderPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                                          Core::RingUploader& uploader, const SceneRenderPass& pass)
    {
        FE_PROFILER_ZONE();

        auto& culled = blackboard.Get<CulledMeshView>();
        ViewCullingState& state = *blackboard.Get<ViewCullingState*>();
        const bool isLate = pass.m_phase == SceneRenderPhase::kLate;
        FE_FG_SCOPE(graph, isLate ? "LateMeshes" : "Meshes");

        // Statistics are complete once the last phase of the reporting pass has culled its meshlets.
        const auto readStats = festd::defer([&] {
            if (pass.m_reportsStats && pass.m_phase != SceneRenderPhase::kEarly)
                ScheduleCullingStatsReadback(graph, state);
        });

        if (!culled.m_classification)
            return;

        FE_Assert(festd::to_underlying(pass.m_phase) == culled.m_phase || isLate);
        if (isLate)
        {
            FE_Assert(pass.m_depthPyramid != nullptr);
            GpuMeshWorkBuilder::CullLateInstances(graph, uploader, *pass.m_depthPyramid, culled);
        }

        MeshPassPhase phase;
        phase.m_phase = festd::to_underlying(pass.m_phase);
        phase.m_depthPyramid = pass.m_depthPyramid;
        phase.m_stats = pass.m_reportsStats ? state.m_stats.Get() : nullptr;

        MeshPipelineRegistry& registry = UpdatePipelineRegistry(graph, uploader, pass.m_techniqueRole);
        PreparedMeshPass prepared;
        const DrawTagMask mask(pass.m_drawTag);
        const Vector2UInt drawTagMask(static_cast<uint32_t>(mask.m_value), static_cast<uint32_t>(mask.m_value >> 32));
        GpuMeshWorkBuilder::Build(graph, uploader, registry, culled, phase, drawTagMask, prepared);
        if (!prepared.m_view)
            return;

        AddGeometryDependencies(graph, prepared.m_view.Get());
        auto* desc = graph.AllocatePassData<MeshPass::PassDesc>();
        const BufferSRVDescriptor viewAddress = graph.GetSRV(prepared.m_view.Get());
        desc->m_pipeline = prepared.m_submissions.front().m_pipeline.Get();
        desc->m_constants = { viewAddress, prepared.m_submissions.front().m_firstCommand };
        desc->m_arguments = Core::BufferView(prepared.m_arguments.Get());

        graph.AddPass(festd::string_view(pass.m_techniqueRole),
                      desc,
                      pass.m_passDescToken,
                      [submissions = std::move(prepared.m_submissions), viewAddress](Core::FrameGraphContext& context) {
                          context.BeginRenderPass();
                          bool firstDispatch = true;
                          for (const auto& submission : submissions)
                          {
                              for (uint32_t page = 0; page < submission.m_commandCount; ++page)
                              {
                                  const MeshPass::Constants constants{ .m_view = viewAddress,
                                                                       .m_commandIndex = submission.m_firstCommand + page };
                                  if (!firstDispatch)
                                  {
                                      context.SetPipeline(submission.m_pipeline.Get());
                                      context.PushConstants(constants);
                                  }

                                  firstDispatch = false;
                                  context.DispatchMeshIndirect(constants.m_commandIndex
                                                               * sizeof(MeshPass::MeshDispatchArguments));
                              }
                          }

                          context.EndRenderPass();
                      });
    }


    MeshSceneModule::ViewCullingState& MeshSceneModule::AcquireViewCullingState(Core::FrameGraph& graph,
                                                                                Core::RingUploader& uploader, const View* view)
    {
        FE_PROFILER_ZONE();

        ViewCullingState* state = nullptr;
        for (const festd::unique_ptr<ViewCullingState>& candidate : m_viewCullingStates)
        {
            if (candidate->m_view == view)
            {
                state = candidate.get();
                break;
            }
        }

        if (state == nullptr)
        {
            state = m_viewCullingStates.emplace_back(festd::make_unique<ViewCullingState>()).get();
            state->m_view = view;
        }

        state->m_lastFrameIndex = m_frameIndex;

        // Visibility is indexed by instance row; new buffers start with everything unseen, so it is tested in the late phase.
        const uint32_t rowCount = Math::Max(m_meshInstanceTable->GetReservedRowCount(), 1u);
        if (state->m_visibilityCapacity < rowCount)
        {
            state->m_visibilityCapacity = Math::CeilPowerOfTwo(rowCount);
            state->m_visibility = CreateZeroedViewBuffer(graph, uploader, "MeshInstanceVisibility", state->m_visibilityCapacity);
        }

        const uint32_t meshletWordCount = Math::Max(m_meshletVisibilityWordCount, 1u);
        if (state->m_meshletVisibilityCapacity < meshletWordCount)
        {
            state->m_meshletVisibilityCapacity = Math::CeilPowerOfTwo(meshletWordCount);
            state->m_meshletVisibility =
                CreateZeroedViewBuffer(graph, uploader, "MeshletVisibility", state->m_meshletVisibilityCapacity);
        }

        constexpr uint32_t kStatsByteSize = MeshPass::CullStat::kCount * sizeof(uint32_t);
        state->m_stats = Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), "MeshCullStats", MeshPass::CullStat::kCount);
        FE_Verify(uploader.UploadGenerated(graph, state->m_stats.Get(), kStatsByteSize, [](void* target) {
            memset(target, 0, kStatsByteSize);
        }));

        return *state;
    }


    void MeshSceneModule::AllocateMeshletVisibility(InstanceRecord& record)
    {
        FreeMeshletVisibility(record);

        uint32_t offset = kInvalidIndex;
        const uint32_t wordCount = record.m_group->m_meshletVisibilityWords;
        if (wordCount != 0)
            record.m_meshletVisibility = m_meshletVisibilityAllocator.Allocate(wordCount);

        if (record.m_meshletVisibility.IsValid())
        {
            offset = record.m_meshletVisibility.m_offset;
            m_meshletVisibilityWordCount = Math::Max(m_meshletVisibilityWordCount, offset + wordCount);
        }

        m_meshInstanceTable->WriteRow(record.m_tableRef).m_meshletVisibilityOffset.Get() = offset;
    }


    void MeshSceneModule::FreeMeshletVisibility(InstanceRecord& record)
    {
        if (!record.m_meshletVisibility.IsValid())
            return;

        m_meshletVisibilityAllocator.Free(record.m_meshletVisibility);
        record.m_meshletVisibility.Invalidate();
    }


    void MeshSceneModule::ReleaseUnusedViewCullingStates()
    {
        const auto isUnused = [this](const festd::unique_ptr<ViewCullingState>& state) {
            if (state->m_lastFrameIndex + kViewCullingStateLifetime > m_frameIndex)
                return false;

            // Keep readback buffers alive until the GPU no longer writes them.
            for (const CullingStatsReadback& readback : state->m_readbacks)
            {
                if (readback.m_isPending && !readback.m_fence.IsReady())
                    return false;
            }

            return true;
        };

        festd::erase_if(m_viewCullingStates, isUnused);
    }


    void MeshSceneModule::ResolveCullingStats(ViewCullingState& state, Culling::Stats* output) const
    {
        // Fences complete in submission order, so the last completed readback holds the newest statistics.
        CullingStatsReadback* latest = nullptr;
        for (CullingStatsReadback& readback : state.m_readbacks)
        {
            if (!readback.m_isPending || !readback.m_fence.IsReady())
                continue;

            readback.m_isPending = false;
            if (latest == nullptr || readback.m_fence.m_value > latest->m_fence.m_value)
                latest = &readback;
        }

        if (latest == nullptr || output == nullptr)
            return;

        uint32_t values[MeshPass::CullStat::kCount];
        const auto* mapped = static_cast<const uint32_t*>(latest->m_buffer->Map());
        latest->m_buffer->InvalidateMappedRange(0, sizeof(values));
        memcpy(values, mapped, sizeof(values));
        latest->m_buffer->Unmap();

        *output = ConvertCullingStats(values);
    }


    void MeshSceneModule::ScheduleCullingStatsReadback(Core::FrameGraph& graph, ViewCullingState& state) const
    {
        // Skip this frame when every readback is still in flight.
        CullingStatsReadback* readback = festd::find_if(state.m_readbacks, [](const CullingStatsReadback& candidate) {
            return !candidate.m_isPending;
        });
        if (readback == state.m_readbacks.end())
            return;

        if (readback->m_buffer == nullptr)
        {
            constexpr uint32_t kStatsByteSize = MeshPass::CullStat::kCount * sizeof(uint32_t);
            readback->m_buffer = Core::Buffer::CreateByteAddress(graph.GetDevice(), "MeshCullStatsReadback", kStatsByteSize);

            Core::ResourceCommitParams commitParams;
            commitParams.m_memory = Core::ResourceMemory::kHostRandomAccess;
            commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            graph.GetResourcePool()->CommitBufferMemory(readback->m_buffer.Get(), commitParams);
        }

        RendererImpl* renderer = Rtti::AssertCast<RendererImpl*>(m_scene->GetRenderer());
        graph.AddCopyPass(Core::BufferView(readback->m_buffer.Get()), Core::BufferView(state.m_stats.Get()));
        readback->m_fence = renderer->GetGraphicsQueue()->GetCurrentFence();
        readback->m_isPending = true;
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
        record.m_transform = &m_meshInstanceTable->WriteRow(sourceIndex).m_transform.Get();
        record.m_transformDirty = false;
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
        record.m_transform = nullptr;
        record.m_transformDirty = false;
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

        FE_PROFILER_ZONE_NAMED("MeshSceneModule::CreateMeshGroup");

        auto* meshGroup = Memory::DefaultNew<MeshGroup>();
        meshGroup->m_asset = meshAsset;
        meshGroup->m_material = material;
        meshGroup->m_tableRef = m_meshGroupTable->AllocateRow();
        meshGroup->m_materialRef = m_materialInstanceTable->AllocateRow();

        const auto materialRow = m_materialInstanceTable->WriteRow(meshGroup->m_materialRef);
        materialRow.m_evaluationSlot.Get() = kInvalidIndex;
        materialRow.m_shadingSlot.Get() = kInvalidIndex;

        m_meshGroups.resize(m_meshGroupTable->GetReservedRowCount());
        m_meshGroups[meshGroup->m_tableRef.m_rowIndex] = meshGroup;
        m_activeMeshGroups.resize(m_meshGroups.size(), false);
        m_activeMeshGroups.set(meshGroup->m_tableRef.m_rowIndex);
        UpdateGroup(meshGroup);
        return meshGroup;
    }


    void MeshSceneModule::UpdateGroup(MeshGroup* group)
    {
        FE_PROFILER_ZONE();

        const IO::AssetRead<MeshAsset> mesh = group->m_asset.Read();
        const IO::AssetRead<MaterialInstanceAsset> material = group->m_material.Read();
        if (!mesh || !material || !material->m_runtime || !mesh->m_buffer)
        {
            if (group->m_materialGeneration != 0)
                ReleaseGroupPipelines(group);

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

            group->m_localBounds = Aabb::kInvalid;
            for (const MeshSubmeshAssetInfo& meshSubmesh : mesh->m_submeshes)
                group->m_localBounds = Math::Union(group->m_localBounds, meshSubmesh.m_bounds);

            const MeshSubmeshAssetInfo& submesh = mesh->m_submeshes[0];
            tableRow.m_bounds.Get().m_min = PackedVector3F(submesh.m_bounds.min);
            tableRow.m_bounds.Get().m_max = PackedVector3F(submesh.m_bounds.max);

            for (const MeshHandle handle : group->m_instances)
                m_boundsDirty.set(m_instances[handle.m_value].m_batch->m_tableRef.m_rowIndex);

            const uint32_t lodCount = submesh.m_lods.size();
            FE_Assert(lodCount > 0 && lodCount <= MeshPass::kMaxLodCount);
            FE_Assert(mesh->m_residentLod < lodCount);

            if (group->m_lodsRef.m_count != lodCount)
            {
                if (group->m_lodsRef.m_count != 0)
                    m_meshLodInfoTable->Free(group->m_lodsRef);
                group->m_lodsRef = m_meshLodInfoTable->AllocateRows(lodCount);
            }

            // Simplification errors are relative to the largest extent of the model that was simplified as a whole.
            const Vector3 modelSize = group->m_localBounds.Size();
            const float errorScale = Math::Max(modelSize.x, Math::Max(modelSize.y, modelSize.z));

            Memory::FiberTempAllocator temp;
            festd::pmr::inline_vector<Core::MeshLodInfo> lodInfos{ &temp };
            festd::pmr::inline_vector<uint32_t> geometryOffsets{ &temp };
            festd::pmr::inline_vector<float> lodErrors{ &temp };
            lodInfos.resize(lodCount);
            geometryOffsets.resize(lodCount);
            lodErrors.resize(lodCount);

            // Assets and their payloads store the least detailed LOD first; table rows start at the most detailed one.
            uint32_t geometryOffset = 0;
            const uint32_t previousMeshletVisibilityWords = group->m_meshletVisibilityWords;
            group->m_maxMeshletCount = 0;
            group->m_meshletVisibilityWords = 0;
            for (uint32_t assetLodIndex = 0; assetLodIndex < lodCount; ++assetLodIndex)
            {
                const MeshLodAssetInfo& assetLod = submesh.m_lods[assetLodIndex];
                FE_Assert(assetLod.m_meshletCount <= Visibility::kMaxMeshletCount);

                const uint32_t lodIndex = lodCount - assetLodIndex - 1;
                Core::MeshLodInfo& lod = lodInfos[lodIndex];
                lod.m_vertexCount = assetLod.m_vertexCount;
                lod.m_indexCount = assetLod.m_indexCount;
                lod.m_meshletCount = assetLod.m_meshletCount;
                lod.m_primitiveCount = assetLod.m_primitiveCount;
                geometryOffsets[lodIndex] = geometryOffset;

                const bool hasError = assetLodIndex < mesh->m_lodErrors.size();
                lodErrors[lodIndex] = hasError ? mesh->m_lodErrors[assetLodIndex] * errorScale : 0.0f;
                if (assetLodIndex <= mesh->m_residentLod)
                    group->m_maxMeshletCount = Math::Max(group->m_maxMeshletCount, assetLod.m_meshletCount);

                const uint32_t visibilityWords = Math::CeilDivide(assetLod.m_meshletCount, MeshPass::kMeshletsPerWorkChunk);
                group->m_meshletVisibilityWords = Math::Max(group->m_meshletVisibilityWords, visibilityWords);

                geometryOffset += CalculateLodPayloadSize(*mesh.Get(), assetLodIndex);
            }

            m_meshLodInfoTable->CopyColumn(group->m_lodsRef, festd::span<const Core::MeshLodInfo>(lodInfos));
            m_meshLodInfoTable->CopyColumn(group->m_lodsRef, festd::span<const uint32_t>(geometryOffsets));
            m_meshLodInfoTable->CopyColumn(group->m_lodsRef, festd::span<const float>(lodErrors));

            const uint32_t finestResidentLod = lodCount - mesh->m_residentLod - 1;
            FE_Assert(geometryOffsets[finestResidentLod] < mesh->m_buffer->GetDesc().m_size);

            tableRow.m_geometry.Get() = BufferPointer{ mesh->m_buffer->GetDeviceAddress() };
            tableRow.m_lods.Get() = group->m_lodsRef;
            tableRow.m_finestResidentLod.Get() = finestResidentLod;
            group->m_buffer = mesh->m_buffer;
            group->m_meshGeneration = meshGeneration;
            group->m_residentLod = mesh->m_residentLod;

            // Reloaded meshes may change their meshlet counts, which invalidates the ranges of existing instances.
            if (group->m_meshletVisibilityWords != previousMeshletVisibilityWords)
            {
                for (const MeshHandle handle : group->m_instances)
                    AllocateMeshletVisibility(m_instances[handle.m_value]);
            }
        }

        if (materialChanged)
        {
            if (group->m_materialGeneration != 0)
                ReleaseGroupPipelines(group);

            const MaterialInstanceTable::RWRow materialRow = m_materialInstanceTable->WriteRow(group->m_materialRef);
            materialRow.m_materialParameters.Get() = material->m_runtime->GetMaterialParameters();
            materialRow.m_evaluationSlot.Get() = m_scene->GetRenderData().m_evaluationPipelines.Register(
                material->m_runtime->GetComputePipeline(Env::Name("SurfaceEvaluate"), m_evaluationWaveLaneCount));
            materialRow.m_shadingSlot.Get() = m_scene->GetRenderData().m_shadingPipelines.Register(
                material->m_runtime->GetComputePipeline(Env::Name("DeferredShade")));
            if (materialRow.m_evaluationSlot.Get() == kInvalidIndex || materialRow.m_shadingSlot.Get() >= 254)
            {
                Logger::LogWarning("Material permutation '{}' has no supported deferred evaluation/shading combination",
                                   material->m_permutationKey);
            }

            tableRow.m_materialInstance.Get() = group->m_materialRef;
            for (const MeshHandle handle : group->m_instances)
            {
                InstanceRecord& record = m_instances[handle.m_value];
                FreeInstanceParameters(group->m_material, record.m_materialGeneration, record.m_parameters);
                record.m_parameters = material->m_runtime->AllocateInstanceParameters();
                record.m_materialGeneration = materialGeneration;
                m_meshInstanceTable->WriteRow(record.m_tableRef).m_instanceData.Get() = record.m_parameters.m_devicePointer;
            }

            group->m_materialGeneration = materialGeneration;
        }

        tableRow.m_renderable.Get() = 1;
    }


    void MeshSceneModule::ReleaseGroupPipelines(MeshGroup* group)
    {
        const auto row = m_materialInstanceTable->WriteRow(group->m_materialRef);
        m_scene->GetRenderData().m_evaluationPipelines.Release(row.m_evaluationSlot.Get());
        m_scene->GetRenderData().m_shadingPipelines.Release(row.m_shadingSlot.Get());
        row.m_evaluationSlot.Get() = kInvalidIndex;
        row.m_shadingSlot.Get() = kInvalidIndex;
        group->m_materialGeneration = 0;
    }


    void MeshSceneModule::DestroyGroup(MeshGroup* group)
    {
        FE_PROFILER_ZONE();

        FE_Assert(group->m_instanceCount == 0);
        ++m_revision;
        m_meshGroups[group->m_tableRef.m_rowIndex] = nullptr;
        m_activeMeshGroups.reset(group->m_tableRef.m_rowIndex);
        if (group->m_lodsRef.m_count != 0)
            m_meshLodInfoTable->Free(group->m_lodsRef);

        ReleaseGroupPipelines(group);
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


    uint32_t MeshSceneModule::GetVisibilityID(const MeshHandle handle) const
    {
        const auto reference = TranslateHandle(handle);
        return reference.m_rowIndex != kInvalidIndex ? (m_visibilityType << 28) | (reference.m_rowIndex + 1) : 0;
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
