#include <Core/IO/AssetReferences.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntitySerialization.h>
#include <Framework/Entities/EntityWorldInternal.h>

namespace FE::Framework
{
    namespace
    {
        constexpr festd::ascii_view kInvalidAsset = "Invalid entity asset";


        Uuid RemapUuid(const festd::span<const EntityUuidBinding> bindings, const Uuid uuid)
        {
            for (const auto& binding : bindings)
            {
                if (binding.m_sourceUuid == uuid)
                    return binding.m_entityUuid;
            }

            return uuid;
        }
    } // namespace


    const EntityCollection& EntityMaterializer::Operation::GetCollection() const
    {
        if (m_collectionRead)
            return *m_collectionRead.Get();

        return m_borrowedCollection ? *m_borrowedCollection : m_collection;
    }


    const EntityCollectionInstanceAsset& EntityMaterializer::Operation::GetDefinition() const
    {
        return m_definitionRead ? *m_definitionRead.Get() : m_definition;
    }


    uint32_t EntityMaterializer::Operation::GetRecordCount() const
    {
        return GetCollection().m_entities.size() + (m_concrete ? 0 : 1);
    }


    const EntityRecord& EntityMaterializer::Operation::GetRecord(uint32_t row) const
    {
        static const EntityRecord kEmptyRoot;
        if (!m_concrete)
        {
            if (row == 0)
                return m_placement ? GetDefinition().m_root.m_entities.front() : kEmptyRoot;

            --row;
        }

        return GetCollection().m_entities[row];
    }


    void EntityMaterializer::Operation::ReleaseDefinition()
    {
        m_definitionRead.Reset();
        m_collectionRead.Reset();
        m_borrowedCollection = nullptr;
        m_request.Reset();
        m_collection = {};
        m_definition = {};
    }


    EntityMaterializer::EntityMaterializer(EntityWorld::Impl& world)
        : m_world(world)
    {
    }


    EntityMaterializer::~EntityMaterializer()
    {
        for (auto* operation : m_operations)
            Memory::DefaultDelete(operation);
    }


    MaterializationToken EntityMaterializer::Add(Operation&& operation)
    {
        FE_Assert(m_operations.size() < UINT32_MAX, "Materialization token space exhausted");

        const MaterializationToken token{ m_world.m_id, m_operations.size() };
        m_operations.push_back(Memory::DefaultNew<Operation>(std::move(operation)));
        if (m_operations.back()->m_state == MaterializationState::kPending)
            m_active.push_back(token.m_index);

        return token;
    }


    EntityMaterializer::Operation* EntityMaterializer::Find(const MaterializationToken token) const
    {
        if (token.m_world != m_world.m_id || token.m_index >= m_operations.size())
            return nullptr;

        return m_operations[token.m_index];
    }


    void EntityMaterializer::Fail(const uint32_t index, const festd::string_view error)
    {
        auto& operation = *m_operations[index];
        operation.m_error = error;
        Cancel({ m_world.m_id, index });
        operation.m_state = MaterializationState::kFailed;
    }


    EntityMaterializer::Operation EntityMaterializer::CreateOperation(EntityRegistry& registry) const
    {
        FE_AssertDebug(Threading::IsMainThread());
        FE_Assert(&registry.GetWorld() == &m_world.m_owner);

        Operation operation;
        operation.m_registry = &registry;
        operation.m_registryId = registry.GetID();
        operation.m_eligibleEpoch = m_world.m_schedule.m_epoch + 1;
        if (!m_world.OwnsRegistry(registry) || registry.m_unloading)
            operation.m_state = MaterializationState::kCanceled;

        return operation;
    }


    MaterializationToken EntityMaterializer::Spawn(EntityRegistry& registry, const EntityCollection& collection)
    {
        Operation operation = CreateOperation(registry);
        operation.m_collection = collection;
        operation.m_hasDefinition = true;
        return Add(std::move(operation));
    }


    MaterializationToken EntityMaterializer::Spawn(EntityRegistry& registry, const IO::AssetID collection)
    {
        Operation operation = CreateOperation(registry);
        operation.m_asset = collection;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollection>(collection));

        return Add(std::move(operation));
    }


    MaterializationToken EntityMaterializer::LoadEntities(EntityRegistry& registry, const EntityCollection& entities,
                                                          const bool validated)
    {
        Operation operation = CreateOperation(registry);
        operation.m_collection = entities;
        operation.m_hasDefinition = true;
        operation.m_concrete = true;
        operation.m_trusted = validated;
        return Add(std::move(operation));
    }


    MaterializationToken EntityMaterializer::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement)
    {
        const auto existing = m_placements.find(placement);
        if (existing != m_placements.end())
            return { m_world.m_id, existing->second };

        Operation operation = CreateOperation(registry);
        operation.m_placement = true;
        operation.m_asset = placement;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollectionInstanceAsset>(placement));

        const auto token = Add(std::move(operation));
        if (m_operations[token.m_index]->m_state == MaterializationState::kPending)
            m_placements[placement] = token.m_index;

        return token;
    }


    MaterializationToken EntityMaterializer::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement,
                                                           const EntityCollectionInstanceAsset& definition,
                                                           const EntityCollection& collection)
    {
        const auto existing = m_placements.find(placement);
        if (existing != m_placements.end())
            return { m_world.m_id, existing->second };

        Operation operation = CreateOperation(registry);
        operation.m_placement = true;
        operation.m_asset = placement;
        operation.m_definition = definition;
        operation.m_collection = collection;
        operation.m_hasDefinition = true;

        const auto token = Add(std::move(operation));
        if (m_operations[token.m_index]->m_state == MaterializationState::kPending)
            m_placements[placement] = token.m_index;

        return token;
    }


    void EntityMaterializer::Cancel(const MaterializationToken token)
    {
        FE_AssertDebug(Threading::IsMainThread());

        Operation* operation = Find(token);
        if (!operation || operation->m_state == MaterializationState::kCanceled)
            return;

        operation->m_state = MaterializationState::kCanceled;
        if (operation->m_placement)
        {
            const auto placement = m_placements.find(operation->m_asset);
            if (placement != m_placements.end() && placement->second == token.m_index)
                m_placements.erase(placement);
        }

        operation->ReleaseDefinition();
        for (const auto id : operation->m_entities)
        {
            if (auto* entity = m_world.m_owner.Find(id))
                m_world.DestroyEntity(*entity);
        }

        operation->m_entities.clear();
        operation->m_schemas.clear();
        operation->m_schemas.shrink_to_fit();
        operation->m_root = {};
    }


    void EntityMaterializer::CancelRegistry(const EntityRegistry& registry)
    {
        for (uint32_t index = 0; index < m_operations.size(); ++index)
        {
            const auto& operation = *m_operations[index];
            if (operation.m_registry == &registry && operation.m_state <= MaterializationState::kReady)
                Cancel({ m_world.m_id, index });
        }
    }


    MaterializationStatus EntityMaterializer::GetStatus(const MaterializationToken token) const
    {
        const Operation* operation = Find(token);
        if (!operation)
            return {};

        return { operation->m_state,
                 operation->m_root,
                 festd::string_view(operation->m_error.data(), operation->m_error.size()) };
    }


    festd::span<const EntityUuidBinding> EntityMaterializer::GetBindings(const MaterializationToken token) const
    {
        const Operation* operation = Find(token);
        return operation ? festd::span<const EntityUuidBinding>(operation->m_bindings) : festd::span<const EntityUuidBinding>{};
    }


    bool EntityMaterializer::HasPending() const
    {
        return eastl::any_of(m_operations.begin(), m_operations.end(), [](const Operation* operation) {
            return operation->m_state == MaterializationState::kPending;
        });
    }


    Archetype* EntityMaterializer::ResolveArchetype(const EntityRecord& record)
    {
        festd::vector<const EntityComponentInfo*> columns;
        for (const auto& component : record.m_components)
        {
            const auto* info = m_world.m_components.Find(component.GetType()->m_id);
            if (!info || info->m_policy.m_transient || !info->m_type->m_deserialize || !info->m_type->m_defaultConstructor)
                return nullptr;

            columns.push_back(info);
        }

        for (uint32_t column = 0; column < columns.size(); ++column)
        {
            for (const auto companion : columns[column]->m_runtimeCompanions)
            {
                const auto* info = m_world.m_components.Find(companion);
                if (festd::find(columns, info) == columns.end())
                    columns.push_back(info);
            }
        }

        festd::sort(columns.begin(), columns.end(), [](const EntityComponentInfo* a, const EntityComponentInfo* b) {
            return a->m_type->m_id < b->m_type->m_id;
        });

        return m_world.GetArchetype(columns);
    }


    bool EntityMaterializer::Validate(Operation& operation)
    {
        const auto& collection = operation.GetCollection();
        const auto& definition = operation.GetDefinition();
        if (operation.m_placement)
        {
            if (!operation.m_asset.IsValid())
                return false;

            // Trusted placements and collections were validated independently; only their pairing remains unchecked.
            const bool valid = operation.m_trusted ? definition.Matches(collection) : definition.Validate(collection);
            if (!valid)
                return false;
        }
        else if (!operation.m_trusted && !collection.Validate())
        {
            return false;
        }

        if (operation.m_placement)
        {
            operation.m_bindings = definition.m_bindings;
        }
        else
        {
            for (const auto& entity : collection.m_entities)
                operation.m_bindings.push_back({ entity.m_uuid, operation.m_concrete ? entity.m_uuid : Uuid::Random() });
        }

        operation.m_rootUuid = operation.m_placement ? definition.m_rootUuid : Uuid::Random();
        if (!operation.m_concrete && m_world.m_owner.Find(operation.m_rootUuid, false))
        {
            operation.m_error = "Placement root UUID is already owned";
            return false;
        }

        for (const auto& binding : operation.m_bindings)
        {
            if (m_world.m_owner.Find(binding.m_entityUuid, false))
            {
                operation.m_error = "Concrete entity UUID is already owned";
                return false;
            }
        }

        for (uint32_t row = 0; row < operation.GetRecordCount(); ++row)
        {
            Archetype* archetype = ResolveArchetype(operation.GetRecord(row));
            if (!archetype)
                return false;

            operation.m_schemas.push_back(archetype);
        }

        operation.m_validated = true;
        return true;
    }


    bool EntityMaterializer::Materialize(const uint32_t index, uint32_t* budget)
    {
        FE_PROFILER_ZONE();

        auto& operation = *m_operations[index];
        const auto reject = [&](const festd::ascii_view error) {
            if (operation.m_error.empty())
                operation.m_error = error;

            operation.m_state = MaterializationState::kFailed;
            return false;
        };

        for (const EntityID id : operation.m_entities)
        {
            if (!m_world.m_owner.Find(id))
                return reject("Materialization member was destroyed");
        }

        // Validate the complete authored definition before allocating any runtime membership.
        if (!operation.m_validated && !Validate(operation))
            return reject(kInvalidAsset);

        bool success = false;
        auto rollback = festd::defer([&] {
            if (success)
                return;

            for (const auto id : operation.m_entities)
            {
                if (auto* entity = m_world.m_owner.Find(id))
                    m_world.DestroyEntity(*entity);
            }

            operation.m_entities.clear();
            operation.m_root = {};
        });

        uint32_t unlimited = UINT32_MAX;
        uint32_t& remaining = budget ? *budget : unlimited;
        const uint32_t recordCount = operation.GetRecordCount();
        for (uint32_t row = operation.m_entities.size(); row < recordCount && remaining; ++row)
        {
            --remaining;
            const auto& record = operation.GetRecord(row);
            const bool generatedRoot = row == 0 && !operation.m_concrete;
            const Uuid uuid = generatedRoot ? operation.m_rootUuid : RemapUuid(operation.m_bindings, record.m_uuid);
            Entity* entity = m_world.AllocateEntity(*operation.m_registry, Env::Name(record.m_name), uuid);
            if (!entity)
                return reject("Concrete entity UUID is already owned");

            entity->m_wantsActive = false;
            entity->m_residencyScope = record.m_residency;

            Archetype& archetype = *operation.m_schemas[row];
            festd::inline_vector<void*, 16> values(archetype.m_columns.size(), nullptr);
            m_world.Migrate(*entity, archetype, values);
            operation.m_entities.push_back(entity->m_id);

            // Copy only authored fields; deserialization hooks remap internal references to this membership.
            for (const auto& component : record.m_components)
            {
                const auto& type = *component.GetType();
                void* value = entity->FindComponent(type.m_id);
                if (!CopyComponent(type, component.GetValue(), value, operation.m_bindings))
                    return reject(kInvalidAsset);

                festd::inline_vector<EntityLifecycle::AssetDependency> dependencies;
                IO::VisitAssetReferences(type, value, [&](const IO::AssetReferenceInfo& reference) {
                    if (reference.m_kind == IO::DependencyKind::kHard)
                        dependencies.emplace_back(reference.m_assetId, reference.m_assetType);
                });

                // Install ownership only after the authored copy succeeds; loading can skip a second discovery pass.
                m_world.m_lifecycle.InstallDependencies(*entity, archetype.Find(type.m_id), dependencies);
            }
        }

        success = true;
        if (operation.m_entities.size() < recordCount)
            return true;

        if (!operation.m_hierarchyInstalled)
        {
            operation.m_root = operation.m_entities.empty() ? EntityID{} : operation.m_entities.front();
            for (uint32_t row = operation.m_concrete ? 0 : 1; row < recordCount; ++row)
            {
                const Uuid parent = operation.GetRecord(row).m_parentUuid;
                Entity* parentEntity = parent.IsValid()
                    ? m_world.m_owner.Find(RemapUuid(operation.m_bindings, parent), false)
                    : (operation.m_concrete ? nullptr : m_world.m_owner.Find(operation.m_root));
                m_world.Reparent(*m_world.m_owner.Find(operation.m_entities[row]), parentEntity);
            }

            operation.m_hierarchyInstalled = true;
        }

        for (; operation.m_loadCount < operation.m_entities.size() && remaining; ++operation.m_loadCount)
        {
            --remaining;
            Entity& entity = *m_world.m_owner.Find(operation.m_entities[operation.m_loadCount]);
            for (uint32_t column = 0; column < entity.m_chunk->m_archetype.m_columns.size(); ++column)
            {
                if (m_world.m_lifecycle.LoadComponent(entity, column) == LifecycleResult::kFailed)
                {
                    success = false;
                    return reject(kInvalidAsset);
                }
            }
        }

        if (operation.m_loadCount < operation.m_entities.size())
            return true;

        for (uint32_t row = 0; row < operation.m_entities.size(); ++row)
        {
            Entity& entity = *m_world.m_owner.Find(operation.m_entities[row]);
            entity.m_wantsActive = operation.GetRecord(row).m_active;
            m_world.m_lifecycle.MarkUnready(entity);
        }

        operation.m_installed = true;
        operation.m_schemas.clear();
        operation.m_schemas.shrink_to_fit();
        return true;
    }


    LifecycleResult EntityMaterializer::ReadDefinition(Operation& operation)
    {
        if (!operation.m_request.IsValid())
            return LifecycleResult::kFailed;

        if (!operation.m_request.IsCompleted())
            return LifecycleResult::kPending;

        if (operation.m_request.GetResult() != IO::AssetLoadResult::kSucceeded)
            return LifecycleResult::kFailed;

        // The asset manager enforced the requested type and ran each asset's Validate() when it was loaded.
        if (operation.m_placement)
        {
            auto placement = IO::AssetHandle<EntityCollectionInstanceAsset>(operation.m_request.GetAssetSlot()).Read();
            if (!placement)
                return LifecycleResult::kFailed;

            // The placement request owns the hard collection link and its complete dependency closure.
            auto collection = placement->m_collection.GetAssetHandle().Read();
            if (!collection)
                return LifecycleResult::kFailed;

            operation.m_definitionRead = std::move(placement);
            operation.m_collectionRead = std::move(collection);
        }
        else
        {
            auto collection = IO::AssetHandle<EntityCollection>(operation.m_request.GetAssetSlot()).Read();
            if (!collection)
                return LifecycleResult::kFailed;

            operation.m_collectionRead = std::move(collection);
        }

        operation.m_trusted = true;
        operation.m_hasDefinition = true;
        return LifecycleResult::kSucceeded;
    }


    void EntityMaterializer::Advance(const bool bootstrap)
    {
        FE_PROFILER_ZONE();

        uint32_t& remaining = m_quota.Refresh(m_world.GetQuotaFrame(), kStepsPerFrame);
        const uint32_t activeCount = m_active.size();
        for (uint32_t active = 0; active < activeCount; ++active)
        {
            const uint32_t index = m_active[active];
            auto& operation = *m_operations[index];

            // A deleted group root retires its live membership.
            const bool rootDestroyed = !operation.m_concrete && !m_world.m_owner.Find(operation.m_root);
            if (operation.m_state == MaterializationState::kReady && rootDestroyed)
                Cancel({ m_world.m_id, index });

            if (operation.m_state != MaterializationState::kPending)
                continue;

            if (!bootstrap && operation.m_eligibleEpoch > m_world.m_schedule.m_epoch)
                continue;

            const bool registryPresent = m_world.OwnsRegistry(*operation.m_registry);
            if (!registryPresent || operation.m_registry->GetID() != operation.m_registryId)
            {
                Cancel({ m_world.m_id, index });
                continue;
            }

            if (operation.m_entities.empty() && !operation.m_hasDefinition)
            {
                const LifecycleResult result = ReadDefinition(operation);
                if (result == LifecycleResult::kPending)
                    continue;

                if (result == LifecycleResult::kFailed)
                {
                    // Copy Core's owned diagnostic before cancellation releases the request.
                    const auto error = operation.m_request.GetError();
                    Fail(index, error.empty() ? festd::string_view("Entity asset unavailable") : error);
                    continue;
                }
            }

            if (operation.m_installed)
                continue;

            if (!bootstrap && remaining == 0)
                continue;

            if (operation.m_concrete && operation.GetCollection().m_entities.empty())
            {
                operation.m_state = MaterializationState::kReady;
                continue;
            }

            if (!Materialize(index, bootstrap ? nullptr : &remaining))
            {
                // Materialize recorded the diagnostic; release whatever membership it created.
                Cancel({ m_world.m_id, index });
                operation.m_state = MaterializationState::kFailed;
                continue;
            }

            if (!operation.m_installed)
                continue;

            // Runtime contributions have been acquired before releasing definition requests.
            operation.ReleaseDefinition();
        }
    }


    void EntityMaterializer::UpdateStatus(const bool bootstrap)
    {
        for (const uint32_t index : m_active)
        {
            auto& operation = *m_operations[index];
            if (operation.m_state != MaterializationState::kPending || !operation.m_installed || operation.m_entities.empty())
                continue;

            bool failed = false;
            bool ready = true;
            for (const auto id : operation.m_entities)
            {
                const auto* entity = m_world.m_owner.Find(id);
                failed |= !entity || entity->HasFailed();

                // Deliberately inactive subtrees do not block readiness.
                bool wantsPublication = entity != nullptr;
                for (const Entity* ancestor = entity; ancestor; ancestor = ancestor->m_parent)
                    wantsPublication &= ancestor->m_wantsActive;

                ready &= entity && (entity->IsActive() || !wantsPublication);
            }

            if (failed)
                Fail(index, "Component lifecycle failed");
            else if (ready)
                operation.m_state = MaterializationState::kReady;
        }

        // Preserve token status history, while polling only pending work and live memberships.
        const auto settled = [&](const uint32_t index) {
            const auto& operation = *m_operations[index];
            if (operation.m_state == MaterializationState::kReady)
                return operation.m_concrete;

            return operation.m_state != MaterializationState::kPending;
        };
        m_active.erase(std::remove_if(m_active.begin(), m_active.end(), settled), m_active.end());

        // Rotate starved work so every operation eventually receives quota.
        if (!bootstrap && m_quota.m_remaining == 0 && m_active.size() > 1)
            eastl::rotate(m_active.begin(), m_active.begin() + 1, m_active.end());
    }


    bool EntityMaterializer::RestoreEntities(EntityRegistry& registry, const EntityCollection& entities,
                                             MaterializationToken& token)
    {
        Operation operation = CreateOperation(registry);
        operation.m_borrowedCollection = &entities;
        operation.m_hasDefinition = true;
        operation.m_concrete = true;
        operation.m_trusted = true;
        token = Add(std::move(operation));

        const bool materialized = Materialize(token.m_index);
        auto& restored = *m_operations[token.m_index];
        restored.ReleaseDefinition();
        if (!materialized)
            return false;

        if (restored.m_entities.empty())
            restored.m_state = MaterializationState::kReady;

        return true;
    }


    void EntityMaterializer::RestorePlacement(EntityRegistry& registry, const EntityPlacementSnapshot& placement)
    {
        Operation operation;
        operation.m_registry = &registry;
        operation.m_registryId = registry.GetID();
        operation.m_asset = placement.m_asset;
        operation.m_placement = true;
        operation.m_state = MaterializationState::kReady;
        operation.m_root = m_world.m_owner.Find(placement.m_rootUuid, false)->GetID();
        operation.m_bindings = placement.m_bindings;
        for (const Uuid uuid : placement.m_members)
            operation.m_entities.push_back(m_world.m_owner.Find(uuid, false)->GetID());

        const auto token = Add(std::move(operation));
        m_placements[placement.m_asset] = token.m_index;
        m_active.push_back(token.m_index);
    }


    void EntityMaterializer::CapturePlacements(const EntityRegistry& registry,
                                               festd::vector<EntityPlacementSnapshot>& placements) const
    {
        // Keep original bindings, including deleted source rows, to prevent accidental source re-expansion.
        for (const auto* operation : m_operations)
        {
            const bool ready = operation->m_state == MaterializationState::kReady;
            if (!operation->m_placement || operation->m_registry != &registry || !ready)
                continue;

            EntityPlacementSnapshot& placement = placements.emplace_back();
            placement.m_asset = operation->m_asset;
            placement.m_rootUuid = m_world.m_owner.Find(operation->m_root)->GetUuid();
            placement.m_bindings = operation->m_bindings;
            for (const auto id : operation->m_entities)
            {
                if (const auto* entity = m_world.m_owner.Find(id))
                    placement.m_members.push_back(entity->GetUuid());
            }
        }
    }
} // namespace FE::Framework
