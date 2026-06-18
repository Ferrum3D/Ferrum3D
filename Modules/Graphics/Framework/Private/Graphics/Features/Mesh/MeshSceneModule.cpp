#include <Core/DI/Activator.h>
#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/RendererImpl.h>
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

        m_meshLodInfoTable = Rc<MeshLodInfoTable>::DefaultNew(database);
        m_meshGroupTable = Rc<MeshGroupTable>::DefaultNew(database);
        m_meshInstanceTable = Rc<MeshInstanceTable>::DefaultNew(database);
    }


    MeshSceneModule::~MeshSceneModule()
    {
        for (MeshBatch* batch : m_batches)
            Memory::DefaultDelete(batch);

        for (const auto& [asset, group] : m_meshGroupsMap)
        {
            FE_Unused(asset);
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

        const DB::Ref<MeshInstanceTable> instanceRef = m_meshInstanceTable->AllocateRow();
        const MeshHandle handle = AllocateHandle(instanceRef);

        MeshGroup* meshGroup = FindOrCreateMeshGroup(desc.m_asset);
        meshGroup->m_instanceCount++;

        const MeshInstanceTable::RWRow instance = m_meshInstanceTable->WriteRow(instanceRef);
        instance.m_meshGroup.Get() = meshGroup->m_tableRef;
        instance.m_transform.Get() = desc.m_transform;

        desc.m_batch->m_meshInstances.push_back(instanceRef);

        return handle;
    }


    void MeshSceneModule::DestroyInstance(const MeshHandle instance)
    {
        const DB::Ref<MeshInstanceTable> tableRef = TranslateHandle(instance);
        m_meshesToDestroy.set(tableRef.m_rowIndex);
        FreeHandle(instance);
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


    MeshGroup* MeshSceneModule::FindOrCreateMeshGroup(ModelAsset* modelAsset)
    {
        const auto it = m_meshGroupsMap.find(modelAsset);
        if (it != m_meshGroupsMap.end())
            return it->second;

        const DB::Ref<MeshGroupTable> tableRef = m_meshGroupTable->AllocateRow();
        const MeshGroupTable::RWRow tableRow = m_meshGroupTable->WriteRow(tableRef);

        const DB::Slice<MeshLodInfoTable> lodsRef = m_meshLodInfoTable->AllocateRows(modelAsset->m_lodCount);

        Memory::FiberTempAllocator temp;
        festd::pmr::inline_vector<Core::MeshLodInfo> lodInfos{ &temp };
        lodInfos.reserve(modelAsset->m_lodCount);
        for (uint32_t lodIndex = 0; lodIndex < modelAsset->m_lodCount; ++lodIndex)
            lodInfos.push_back(modelAsset->GetLodInfo(0, lodIndex));

        m_meshLodInfoTable->CopyColumn(lodsRef, lodInfos);

        Core::DescriptorManager* descriptorManager = Env::GetServiceProvider()->ResolveRequired<Core::DescriptorManager>();
        const uint32_t descriptorIndex = descriptorManager->ReserveDescriptor(modelAsset->GetGeometryBuffer(0));
        descriptorManager->CommitResourceDescriptor(descriptorIndex, Core::DescriptorType::kSRV);

        tableRow.m_geometry.Get() = BufferPointer{ descriptorManager->GetDeviceAddress(descriptorIndex) };
        tableRow.m_lods.Get() = lodsRef;

        auto* meshGroup = Memory::DefaultNew<MeshGroup>();
        meshGroup->m_asset = modelAsset;
        meshGroup->m_tableRef = tableRef;
        m_meshGroupsMap.emplace(modelAsset, meshGroup);

        m_meshGroups.resize(m_meshGroupTable->GetReservedRowCount());
        m_meshGroups[tableRef.m_rowIndex] = meshGroup;

        return meshGroup;
    }


    ModelAsset* MeshSceneModule::FindAsset(const DB::Ref<MeshGroupTable> group) const
    {
        if (group.m_rowIndex >= m_meshGroups.size())
            return nullptr;

        return m_meshGroups[group.m_rowIndex]->m_asset;
    }


    DB::Ref<MeshInstanceTable> MeshSceneModule::TranslateHandle(const MeshHandle handle) const
    {
        if (handle.IsValid())
            return m_handleTranslationTable[handle.m_value];

        return DB::Ref<MeshInstanceTable>::CreateInvalid();
    }
} // namespace FE::Graphics
