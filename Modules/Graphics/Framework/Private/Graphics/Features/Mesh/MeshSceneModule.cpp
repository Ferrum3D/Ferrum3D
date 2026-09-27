#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/RendererImpl.h>
#include <Graphics/Tables/MaterialInstanceTable.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

namespace FE::Graphics
{
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
    }


    MeshSceneModule::~MeshSceneModule()
    {
        while (!m_batches.empty())
            DestroyBatch(m_batches.back());

        for (MeshGroup* group : m_meshGroups)
        {
            if (group != nullptr)
                DestroyGroup(group);
        }
    }


    MeshBatch* MeshSceneModule::CreateBatch(const MeshBatchDesc& desc)
    {
        FE_Assert(desc.m_bounds.IsValid());
        auto* batch = Memory::DefaultNew<MeshBatch>();
        batch->m_parent = this;
        batch->m_drawTagMask = desc.m_drawTagMask;
        batch->m_octreeEntry.m_bounds = desc.m_bounds;
        batch->m_octreeEntry.m_userIndex = m_batches.size();
        m_octree.InsertOrUpdate(batch->m_octreeEntry);
        m_batches.push_back(batch);
        return batch;
    }


    void MeshSceneModule::DestroyBatch(MeshBatch* batch)
    {
        if (batch == nullptr)
            return;

        const auto it = festd::find(m_batches, batch);
        if (it == m_batches.end())
            return;

        while (!batch->m_handles.empty())
            DestroyInstance(batch->m_handles.back());

        m_octree.Remove(batch->m_octreeEntry);
        const uint32_t batchIndex = static_cast<uint32_t>(it - m_batches.begin());
        m_batches.erase_unsorted(it);
        if (batchIndex < m_batches.size())
            m_batches[batchIndex]->m_octreeEntry.m_userIndex = batchIndex;
        Memory::DefaultDelete(batch);
    }


    MeshHandle MeshSceneModule::CreateInstance(const MeshInstanceDesc& desc)
    {
        FE_Assert(desc.m_asset.IsValid() && desc.m_asset.IsReady());
        FE_Assert(desc.m_material.IsValid() && desc.m_material.IsReady());
        FE_Assert(desc.m_batch != nullptr && desc.m_batch->m_parent == this);

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

        return handle;
    }


    void MeshSceneModule::DestroyInstance(const MeshHandle handle)
    {
        if (!handle.IsValid() || handle.m_value >= m_instances.size())
            return;

        InstanceRecord& record = m_instances[handle.m_value];
        if (record.m_generation != handle.m_generation || record.m_tableRef.m_rowIndex == kInvalidIndex)
            return;

        const IO::AssetRead<MaterialInstanceAsset> material = record.m_group->m_material.Read();
        const bool currentMaterialGeneration = record.m_materialGeneration == record.m_group->m_material.GetGeneration();
        if (currentMaterialGeneration && material && material->m_runtime && record.m_parameters.IsValid())
            material->m_runtime->FreeInstanceParameters(record.m_parameters);

        const auto batchRef = festd::find_if(record.m_batch->m_meshInstances, [&](const DB::Ref<MeshInstanceTable> ref) {
            return ref.m_rowIndex == record.m_tableRef.m_rowIndex;
        });
        FE_Assert(batchRef != record.m_batch->m_meshInstances.end());
        record.m_batch->m_meshInstances.erase_unsorted(batchRef);
        const auto batchHandle = festd::find(record.m_batch->m_handles, handle);
        FE_Assert(batchHandle != record.m_batch->m_handles.end());
        record.m_batch->m_handles.erase_unsorted(batchHandle);

        MeshGroup* group = record.m_group;
        const auto groupHandle = festd::find(group->m_instances, handle);
        FE_Assert(groupHandle != group->m_instances.end());
        group->m_instances.erase_unsorted(groupHandle);
        --group->m_instanceCount;

        m_meshInstanceTable->Free(record.m_tableRef);
        FreeHandle(handle);
        if (group->m_instanceCount == 0)
            DestroyGroup(group);
    }


    void MeshSceneModule::Update()
    {
        for (MeshGroup* group : m_meshGroups)
        {
            if (group != nullptr)
                UpdateGroup(group);
        }
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
        UpdateGroup(meshGroup);
        return meshGroup;
    }


    void MeshSceneModule::UpdateGroup(MeshGroup* group)
    {
        const IO::AssetRead<MeshAsset> mesh = group->m_asset.Read();
        const IO::AssetRead<MaterialInstanceAsset> material = group->m_material.Read();
        if (!mesh || !material || !material->m_runtime || !mesh->m_buffer)
            return;

        const uint32_t meshGeneration = group->m_asset.GetGeneration();
        const uint32_t materialGeneration = group->m_material.GetGeneration();
        const bool meshChanged = group->m_meshGeneration != meshGeneration || group->m_buffer != mesh->m_buffer.Get()
            || group->m_residentLod != mesh->m_residentLod;
        const bool materialChanged = group->m_materialGeneration != materialGeneration;
        if (!meshChanged && !materialChanged)
            return;

        const MeshGroupTable::RWRow tableRow = m_meshGroupTable->WriteRow(group->m_tableRef);
        if (meshChanged)
        {
            FE_Assert(!mesh->m_submeshes.empty());
            const MeshSubmeshAssetInfo& submesh = mesh->m_submeshes[0];
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
                    geometryOffset += uint64_t(info.m_vertexCount) * mesh->m_vertexStride;
                    geometryOffset += uint64_t(info.m_indexCount) * sizeof(uint32_t);
                    geometryOffset += uint64_t(info.m_meshletCount) * sizeof(Core::MeshletHeader);
                    geometryOffset += uint64_t(info.m_primitiveCount) * sizeof(Core::PackedTriangle);
                }
            }
            FE_Assert(geometryOffset < mesh->m_buffer->GetDesc().m_size);

            Core::DescriptorManager* descriptorManager = Renderer::Get().GetDescriptorManager();
            const uint32_t descriptorIndex = descriptorManager->ReserveDescriptor(mesh->m_buffer.Get());
            descriptorManager->CommitResourceDescriptor(descriptorIndex, Core::DescriptorType::kSRV);
            tableRow.m_geometry.Get() = BufferPointer{ descriptorManager->GetDeviceAddress(descriptorIndex) + geometryOffset };
            tableRow.m_lods.Get() = DB::Slice<MeshLodInfoTable>{ group->m_lodsRef.m_rowIndex + lodIndex, 1 };
            group->m_buffer = mesh->m_buffer.Get();
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
    }


    void MeshSceneModule::DestroyGroup(MeshGroup* group)
    {
        FE_Assert(group->m_instanceCount == 0);
        m_meshGroups[group->m_tableRef.m_rowIndex] = nullptr;
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
