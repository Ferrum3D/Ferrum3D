#pragma once
#include <Framework/Entities/EntityLifecycle.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <festd/string.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    //! @brief Turn collection, placement and concrete definitions into runtime entities across safe points.
    //! Operations are indexed by token and retained for status history until the world is destroyed.
    struct EntityMaterializer final
    {
        static constexpr uint32_t kStepsPerFrame = 32;

        explicit EntityMaterializer(EntityWorld::Impl& world);
        ~EntityMaterializer();

        EntityMaterializer(const EntityMaterializer&) = delete;
        EntityMaterializer& operator=(const EntityMaterializer&) = delete;

        MaterializationToken Spawn(EntityRegistry& registry, const EntityCollection& collection);
        MaterializationToken Spawn(EntityRegistry& registry, IO::AssetID collection);
        //! @brief Queue concrete entities that keep their UUIDs and receive no generated root.
        //! Pass validated when the caller already validated the entities as part of a larger definition.
        MaterializationToken LoadEntities(EntityRegistry& registry, const EntityCollection& entities, bool validated = false);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement,
                                           const EntityCollectionInstanceAsset& definition, const EntityCollection& collection);

        void Cancel(MaterializationToken token);
        //! @brief Cancel pending and ready operations owned by a registry that is being removed.
        void CancelRegistry(const EntityRegistry& registry);

        [[nodiscard]] MaterializationStatus GetStatus(MaterializationToken token) const;
        [[nodiscard]] festd::span<const EntityUuidBinding> GetBindings(MaterializationToken token) const;
        [[nodiscard]] bool HasPending() const;

        //! @brief Poll definition requests and construct rows within the frame quota; bootstrap is unlimited.
        void Advance(bool bootstrap);
        //! @brief Publish readiness after lifecycle advancement and stop polling settled operations.
        void UpdateStatus(bool bootstrap);

        //! @brief Construct validated concrete snapshot rows immediately without copying them; lifecycle may remain asynchronous.
        bool RestoreEntities(EntityRegistry& registry, const EntityCollection& entities, MaterializationToken& token);
        //! @brief Recreate the bookkeeping of a ready placement whose members were restored concretely.
        void RestorePlacement(EntityRegistry& registry, const EntityPlacementSnapshot& placement);
        //! @brief Describe ready placements of a registry for a snapshot.
        void CapturePlacements(const EntityRegistry& registry, festd::vector<EntityPlacementSnapshot>& placements) const;

    private:
        struct Operation
        {
            //! @brief Owned definition copies for in-memory requests.
            EntityCollectionInstanceAsset m_definition;
            EntityCollection m_collection;
            //! @brief Published definitions pinned while rows are constructed; the request keeps them resident.
            IO::AssetRead<EntityCollectionInstanceAsset> m_definitionRead;
            IO::AssetRead<EntityCollection> m_collectionRead;
            //! @brief Caller-owned collection, valid only during a synchronous restore.
            const EntityCollection* m_borrowedCollection = nullptr;

            IO::AssetID m_asset = IO::AssetID::kNull;
            festd::vector<EntityUuidBinding> m_bindings;
            festd::vector<EntityID> m_entities;
            festd::vector<Archetype*> m_schemas;
            Uuid m_rootUuid = Uuid::kNull;
            uint32_t m_loadCount = 0;
            festd::string m_error;
            IO::AssetRequest m_request;
            EntityRegistry* m_registry = nullptr;
            uint64_t m_eligibleEpoch = 0;
            EntityID m_root;
            uint32_t m_registryId = 0;
            MaterializationState m_state = MaterializationState::kPending;
            //! @brief Rows keep their authored UUIDs and are not grouped beneath a generated root.
            bool m_concrete = false;
            bool m_placement = false;
            bool m_hasDefinition = false;
            //! @brief The definition content was already validated (by the asset manager or a whole-world check).
            bool m_trusted = false;
            bool m_validated = false;
            bool m_installed = false;
            bool m_hierarchyInstalled = false;

            [[nodiscard]] const EntityCollection& GetCollection() const;
            [[nodiscard]] const EntityCollectionInstanceAsset& GetDefinition() const;
            //! @brief Number of runtime rows, including the generated root of non-concrete operations.
            [[nodiscard]] uint32_t GetRecordCount() const;
            [[nodiscard]] const EntityRecord& GetRecord(uint32_t row) const;
            //! @brief Drop definition copies, pins and the definition request once rows own their dependencies.
            void ReleaseDefinition();
        };

        Operation CreateOperation(EntityRegistry& registry) const;
        MaterializationToken Add(Operation&& operation);
        Operation* Find(MaterializationToken token) const;
        void Fail(uint32_t index, festd::string_view error);

        //! @brief Read a completed definition request into the operation.
        LifecycleResult ReadDefinition(Operation& operation);
        //! @brief Validate the complete definition and resolve every row layout before allocating rows.
        bool Validate(Operation& operation);
        Archetype* ResolveArchetype(const EntityRecord& record);
        bool Materialize(uint32_t index, uint32_t* budget = nullptr);

        EntityWorld::Impl& m_world;
        festd::vector<Operation*> m_operations;
        //! @brief Operations that are pending or own live membership.
        festd::vector<uint32_t> m_active;
        festd::unordered_dense_map<IO::AssetID, uint32_t> m_placements;
        FrameQuota m_quota;
    };
} // namespace FE::Framework
