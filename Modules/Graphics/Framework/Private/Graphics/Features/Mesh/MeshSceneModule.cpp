#include <Graphics/Features/Mesh/MeshSceneModule.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>

namespace FE::Graphics
{
    MeshSceneModule::MeshSceneModule(Scene* scene)
        : SceneModuleBase(scene)
    {
    }


    MeshSceneModule::~MeshSceneModule() = default;


    MeshHandle MeshSceneModule::CreateInstance(const MeshInstanceDesc& desc)
    {
        const DB::Ref<MeshInstanceTable> instanceRef = m_meshInstanceTable->AllocateRow();
        const MeshHandle handle = AllocateHandle(instanceRef);

        MeshGroup* meshGroup = FindMeshGroup(desc.m_asset);
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


    MeshGroup* MeshSceneModule::FindMeshGroup(ModelAsset* modelAsset)
    {
        const auto it = m_meshGroups.find(modelAsset);
        if (it != m_meshGroups.end())
            return it->second;

        const DB::Ref<MeshGroupTable> tableRef = m_meshGroupTable->AllocateRow();
        const MeshGroupTable::RWRow tableRow = m_meshGroupTable->WriteRow(tableRef);

        const DB::Slice<MeshLodInfoTable> lodsRef = m_meshLodInfoTable->AllocateRows(modelAsset->m_lodCount);
        m_meshLodInfoTable->CopyColumn(lodsRef, modelAsset->m_lods);

        tableRow.m_lods.Get() = lodsRef;

        auto* meshGroup = Memory::DefaultNew<MeshGroup>();
        meshGroup->m_asset = modelAsset;
        meshGroup->m_tableRef = tableRef;
        return meshGroup;
    }


    DB::Ref<MeshInstanceTable> MeshSceneModule::TranslateHandle(const MeshHandle handle) const
    {
        if (handle.IsValid())
            return m_handleTranslationTable[handle.m_value];

        return DB::Ref<MeshInstanceTable>::CreateInvalid();
    }
} // namespace FE::Graphics
