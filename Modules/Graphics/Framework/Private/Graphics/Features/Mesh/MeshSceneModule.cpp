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
        for (MeshBatch* batch : m_batches)
            Memory::DefaultDelete(batch);

        for (MeshGroup* group : m_meshGroups)
        {
            if (group != nullptr)
                Memory::DefaultDelete(group);
        }
    }


    MeshBatch* MeshSceneModule::CreateBatch(const MeshBatchDesc& desc)
    {
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
        const auto it = festd::find(m_batches, batch);
        if (it == m_batches.end())
            return;

        m_batches.erase_unsorted(it);
        Memory::DefaultDelete(batch);
    }


    MeshHandle MeshSceneModule::CreateInstance(const MeshInstanceDesc& desc)
    {
        FE_Assert(desc.m_asset != nullptr);
        FE_Assert(desc.m_batch != nullptr);
        FE_Assert(desc.m_material != nullptr);

        const DB::Ref<MeshInstanceTable> instanceRef = m_meshInstanceTable->AllocateRow();
        const MeshHandle handle = AllocateHandle(instanceRef);

        MeshGroup* meshGroup = FindOrCreateMeshGroup(desc.m_asset, desc.m_material);
        meshGroup->m_instanceCount++;

        const MeshInstanceTable::RWRow instance = m_meshInstanceTable->WriteRow(instanceRef);
        instance.m_meshGroup.Get() = meshGroup->m_tableRef;
        instance.m_transform.Get() = desc.m_transform;
        instance.m_instanceData.Get() = desc.m_instanceData;

        desc.m_batch->m_meshInstances.push_back(instanceRef);

        return handle;
    }


    void MeshSceneModule::DestroyInstance(const MeshHandle instance)
    {
        const DB::Ref<MeshInstanceTable> tableRef = TranslateHandle(instance);
        m_meshesToDestroy.set(tableRef.m_rowIndex);
        FreeHandle(instance);
    }


    void MeshSceneModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    MeshHandle MeshSceneModule::AllocateHandle(const DB::Ref<MeshInstanceTable> sourceIndex)
    {
        EnsureCapacity();

        const uint32_t handleIndex = m_freeHandles.find_first();
        FE_Assert(handleIndex != kInvalidIndex);
        m_freeHandles.reset(handleIndex);

        m_handleTranslationTable[handleIndex] = sourceIndex;
        return MeshHandle{ handleIndex };
    }


    void MeshSceneModule::FreeHandle(const MeshHandle handle)
    {
        m_handleTranslationTable[handle.m_value].Invalidate();
        m_freeHandles.set(handle.m_value);
    }


    void MeshSceneModule::EnsureCapacity()
    {
        const uint32_t capacity = m_meshInstanceTable->GetReservedRowCount();
        if (m_freeHandles.size() < capacity)
        {
            m_freeHandles.resize(capacity, true);
            m_handleTranslationTable.resize(capacity, DB::Ref<MeshInstanceTable>::CreateInvalid());

            m_meshesToDestroy.resize(capacity, false);
        }
    }


    MeshGroup* MeshSceneModule::FindOrCreateMeshGroup(const MeshAsset* meshAsset, MaterialInstanceRuntime* material)
    {
        for (MeshGroup* group : m_meshGroups)
        {
            if (group != nullptr && group->m_asset == meshAsset && group->m_material == material)
                return group;
        }

        FE_Assert(!meshAsset->m_submeshes.empty());
        FE_Assert(meshAsset->m_buffer);
        const MeshSubmeshAssetInfo& submesh = meshAsset->m_submeshes[0];

        const DB::Ref<MeshGroupTable> tableRef = m_meshGroupTable->AllocateRow();
        const MeshGroupTable::RWRow tableRow = m_meshGroupTable->WriteRow(tableRef);

        const DB::Slice<MeshLodInfoTable> lodsRef = m_meshLodInfoTable->AllocateRows(submesh.m_lods.size());

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

        m_meshLodInfoTable->CopyColumn(lodsRef, lodInfos);

        Core::DescriptorManager* descriptorManager = Renderer::Get().GetDescriptorManager();
        const uint32_t descriptorIndex = descriptorManager->ReserveDescriptor(meshAsset->m_buffer.Get());
        descriptorManager->CommitResourceDescriptor(descriptorIndex, Core::DescriptorType::kSRV);

        tableRow.m_geometry.Get() = BufferPointer{ descriptorManager->GetDeviceAddress(descriptorIndex) };
        tableRow.m_lods.Get() = lodsRef;

        const DB::Ref<MaterialInstanceTable> materialRef = m_materialInstanceTable->AllocateRow();
        m_materialInstanceTable->WriteRow(materialRef).m_materialParameters.Get() = material->GetMaterialParameters();
        tableRow.m_materialInstance.Get() = materialRef;

        auto* meshGroup = Memory::DefaultNew<MeshGroup>();
        meshGroup->m_asset = meshAsset;
        meshGroup->m_material = material;
        meshGroup->m_tableRef = tableRef;

        m_meshGroups.resize(m_meshGroupTable->GetReservedRowCount());
        m_meshGroups[tableRef.m_rowIndex] = meshGroup;

        return meshGroup;
    }


    const MeshAsset* MeshSceneModule::FindAsset(const DB::Ref<MeshGroupTable> group) const
    {
        if (group.m_rowIndex >= m_meshGroups.size() || m_meshGroups[group.m_rowIndex] == nullptr)
            return nullptr;

        return m_meshGroups[group.m_rowIndex]->m_asset;
    }


    MaterialInstanceRuntime* MeshSceneModule::FindMaterial(const DB::Ref<MeshGroupTable> group) const
    {
        if (group.m_rowIndex >= m_meshGroups.size() || m_meshGroups[group.m_rowIndex] == nullptr)
            return nullptr;
        return m_meshGroups[group.m_rowIndex]->m_material;
    }


    DB::Ref<MeshInstanceTable> MeshSceneModule::TranslateHandle(const MeshHandle handle) const
    {
        if (handle.IsValid())
            return m_handleTranslationTable[handle.m_value];

        return DB::Ref<MeshInstanceTable>::CreateInvalid();
    }
} // namespace FE::Graphics
