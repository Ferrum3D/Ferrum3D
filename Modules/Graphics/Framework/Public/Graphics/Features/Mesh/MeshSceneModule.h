#pragma once
#include <Core/Math/Sphere.h>
#include <Graphics/Assets/Assets.h>
#include <Graphics/Assets/MaterialAssets.h>
#include <Graphics/Base/DrawTag.h>
#include <Graphics/Database/Base.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>
#include <Graphics/Scene/Octree.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Tables/Forwards.h>
#include <Graphics/Tables/MeshBatchTable.h>
#include <Graphics/Tables/MeshLodInfoTable.h>
#include <Graphics/Tables/MeshMemberTable.h>
#include <festd/unordered_map.h>

namespace FE::Graphics
{
    struct MeshSceneModule;

    struct MeshHandle final
    {
        uint32_t m_value = kInvalidIndex;
        uint32_t m_generation = 0;

        [[nodiscard]] bool IsValid() const
        {
            return m_value != kInvalidIndex;
        }

        friend bool operator==(const MeshHandle& lhs, const MeshHandle& rhs)
        {
            return lhs.m_value == rhs.m_value && lhs.m_generation == rhs.m_generation;
        }
    };

    struct MeshBatch final
    {
        static constexpr uint32_t kMaxInstanceCount = MeshMemberTable::kRowsPerPage;

        OctreeEntry m_octreeEntry;
        MeshSceneModule* m_parent = nullptr;
        DrawTagMask m_drawTagMask;
        festd::vector<DB::Ref<MeshInstanceTable>> m_meshInstances;
        festd::vector<MeshHandle> m_handles;
        DB::Ref<MeshBatchTable> m_tableRef;
        DB::Slice<MeshMemberTable> m_members{};
    };


    struct MeshGroup final
    {
        uint32_t m_instanceCount = 0;
        DB::Ref<MeshGroupTable> m_tableRef = DB::Ref<MeshGroupTable>::CreateInvalid();
        DB::Ref<MaterialInstanceTable> m_materialRef = DB::Ref<MaterialInstanceTable>::CreateInvalid();
        DB::Slice<MeshLodInfoTable> m_lodsRef{};
        IO::AssetLease<MeshAsset> m_asset;
        IO::AssetLease<MaterialInstanceAsset> m_material;
        Rc<Core::Buffer> m_buffer;
        uint32_t m_meshGeneration = 0;
        uint32_t m_materialGeneration = kInvalidIndex;
        uint32_t m_residentLod = kInvalidIndex;
        festd::vector<MeshHandle> m_instances;
    };


    struct MeshInstanceDesc final
    {
        IO::AssetLease<MeshAsset> m_asset;
        MeshBatch* m_batch = nullptr;
        IO::AssetLease<MaterialInstanceAsset> m_material;
        Matrix4x4 m_transform;
    };


    struct MeshBatchDesc final
    {
        Aabb m_bounds = Aabb::kInvalid;
        DrawTagMask m_drawTagMask;
    };


    struct MeshSceneModule final : public SceneModuleBase
    {
        FE_RTTI("1784843C-5085-4289-AA82-04C480E423EE");

        explicit MeshSceneModule(Scene* scene);
        ~MeshSceneModule() override;

        [[nodiscard]] MeshBatch* CreateBatch(const MeshBatchDesc& desc);
        void DestroyBatch(MeshBatch* batch);
        void SetBatchDrawTags(MeshBatch* batch, DrawTagMask tags);

        [[nodiscard]] MeshHandle CreateInstance(const MeshInstanceDesc& desc);
        void DestroyInstance(MeshHandle instance);

        void Update() override;

        void UpdateTransform(MeshHandle instance, const Matrix4x4& transform);
        void MoveInstance(MeshHandle instance, MeshBatch* batch);
        void UpdateMaterial(MeshHandle handle, const IO::AssetLease<MaterialInstanceAsset>& materialLease);

        void UpdateRenderData(Core::FrameGraph& graph, Core::RingUploader& uploader) override;
        void PrepareRenderView(Core::FrameGraph& graph, Core::RingUploader& uploader) override;
        void AddRenderPasses(Core::FrameGraph& graph, Core::RingUploader& uploader, const SceneRenderPass& pass) override;

        // Indexed batch slots; destroyed batches leave null pointers until their slots are reused.
        [[nodiscard]] const festd::vector<MeshBatch*>& GetBatches() const
        {
            return m_batches;
        }

        [[nodiscard]] IO::AssetRead<MeshAsset> FindAsset(DB::Ref<MeshGroupTable> group) const;

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

        [[nodiscard]] MaterialInstanceTable* GetMaterialInstanceTable() const
        {
            return m_materialInstanceTable.Get();
        }

        [[nodiscard]] IO::AssetRead<MaterialInstanceAsset> FindMaterial(DB::Ref<MeshGroupTable> group) const;

    private:
        void DoRelease() override;

        struct InstanceRecord final
        {
            DB::Ref<MeshInstanceTable> m_tableRef = DB::Ref<MeshInstanceTable>::CreateInvalid();
            MeshBatch* m_batch = nullptr;
            MeshGroup* m_group = nullptr;
            MaterialParameterAllocator::Allocation m_parameters;
            uint32_t m_materialGeneration = 0;
            uint32_t m_generation = 1;
        };

        MeshHandle AllocateHandle(DB::Ref<MeshInstanceTable> sourceIndex, MeshBatch* batch, MeshGroup* group,
                                  MaterialParameterAllocator::Allocation parameters);
        void FreeHandle(MeshHandle handle);
        void EnsureCapacity();
        void DestroyGroup(MeshGroup* group);
        void UpdateGroup(MeshGroup* group);
        MeshPipelineRegistry& UpdatePipelineRegistry(Core::FrameGraph& graph, Core::RingUploader& uploader, Env::Name role);
        void UpdateBatchBounds(MeshBatch* batch);
        void UpdateBatchMembership(MeshBatch* batch);

        MeshGroup* FindOrCreateMeshGroup(const IO::AssetLease<MeshAsset>& meshAsset,
                                         const IO::AssetLease<MaterialInstanceAsset>& material);

        DB::Ref<MeshInstanceTable> TranslateHandle(MeshHandle handle) const;

        festd::bit_vector m_freeHandles;
        festd::vector<InstanceRecord> m_instances;

        festd::vector<MeshGroup*> m_meshGroups;
        festd::vector<MeshBatch*> m_batches;
        festd::bit_vector m_activeMeshGroups;
        festd::bit_vector m_boundsDirty;
        festd::bit_vector m_membershipDirty;

        Rc<MeshLodInfoTable> m_meshLodInfoTable;
        Rc<MeshGroupTable> m_meshGroupTable;
        Rc<MeshInstanceTable> m_meshInstanceTable;
        Rc<MaterialInstanceTable> m_materialInstanceTable;
        Rc<MeshBatchTable> m_batchTable;
        Rc<MeshMemberTable> m_memberTable;
        festd::vector<MeshPipelineRegistry> m_registries;
        uint64_t m_revision = 1;
        Octree m_octree;
    };
} // namespace FE::Graphics
