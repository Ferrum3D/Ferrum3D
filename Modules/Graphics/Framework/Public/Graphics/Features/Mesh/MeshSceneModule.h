#pragma once
#include <FeCore/Math/Sphere.h>
#include <Graphics/Assets/IModelAssetManager.h>
#include <Graphics/Base/DrawTag.h>
#include <Graphics/Database/Base.h>
#include <Graphics/Scene/Octree.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Tables/Forwards.h>
#include <festd/unordered_map.h>

namespace FE::Graphics
{
    struct MeshSceneModule;

    struct MeshBatch final
    {
        OctreeEntry m_octreeEntry;
        MeshSceneModule* m_parent = nullptr;
        DrawTagMask m_drawTagMask;
        festd::vector<DB::Ref<MeshInstanceTable>> m_meshInstances;
    };


    struct MeshGroup final
    {
        uint32_t m_instanceCount = 0;
        DB::Ref<MeshGroupTable> m_tableRef = DB::Ref<MeshGroupTable>::CreateInvalid();
        ModelAsset* m_asset = nullptr;
    };


    struct MeshInstanceDesc final
    {
        ModelAsset* m_asset = nullptr;
        MeshBatch* m_batch = nullptr;
        Matrix4x4 m_transform;
    };


    struct MeshBatchDesc final
    {
        Aabb m_bounds = Aabb::kInvalid;
        DrawTagMask m_drawTagMask;
    };


    struct MeshHandle final : public TypedHandle<MeshHandle, uint32_t>
    {
    };


    struct MeshSceneModule final : public SceneModuleBase
    {
        FE_RTTI("1784843C-5085-4289-AA82-04C480E423EE");

        explicit MeshSceneModule(Scene* scene);
        ~MeshSceneModule() override;

        [[nodiscard]] MeshBatch* CreateBatch(const MeshBatchDesc& desc);
        void DestroyBatch(MeshBatch* batch);

        [[nodiscard]] MeshHandle CreateInstance(const MeshInstanceDesc& desc);
        void DestroyInstance(MeshHandle instance);

        [[nodiscard]] const festd::vector<MeshBatch*>& GetBatches() const
        {
            return m_batches;
        }

        [[nodiscard]] MeshBatch* FindBatch(DB::Ref<MeshInstanceTable> instance) const;

        [[nodiscard]] MeshInstanceTable* GetMeshInstanceTable() const
        {
            return m_meshInstanceTable.Get();
        }

        [[nodiscard]] MeshGroupTable* GetMeshGroupTable() const
        {
            return m_meshGroupTable.Get();
        }

        [[nodiscard]] MeshLodInfoTable* GetMeshLodInfoTable() const
        {
            return m_meshLodInfoTable.Get();
        }

    private:
        MeshHandle AllocateHandle(DB::Ref<MeshInstanceTable> sourceIndex);
        void FreeHandle(MeshHandle handle);
        void EnsureCapacity();

        MeshGroup* FindMeshGroup(ModelAsset* modelAsset);

        DB::Ref<MeshInstanceTable> TranslateHandle(MeshHandle handle) const;

        festd::bit_vector m_freeHandles;
        festd::vector<DB::Ref<MeshInstanceTable>> m_handleTranslationTable;

        festd::unordered_dense_map<ModelAsset*, MeshGroup*> m_meshGroups;
        festd::vector<MeshBatch*> m_batches;

        festd::bit_vector m_meshesToDestroy;

        Rc<MeshLodInfoTable> m_meshLodInfoTable;
        Rc<MeshGroupTable> m_meshGroupTable;
        Rc<MeshInstanceTable> m_meshInstanceTable;
        Octree m_octree;
    };
} // namespace FE::Graphics
