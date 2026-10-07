#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    MaterializationToken EntityWorld::SpawnCollection(EntityRegistry& registry, const EntityCollection& collection)
    {
        FE_Assert(Threading::IsMainThread());
        FE_Assert(m_impl->m_materializations.size() < UINT32_MAX, "Materialization token space exhausted");
        Impl::Materialization operation;
        operation.m_registry = &registry;
        operation.m_registryId = registry.GetID();
        operation.m_eligibleEpoch = m_impl->m_epoch + 1;
        const bool registryOwned = festd::find(m_impl->m_registries, &registry) != m_impl->m_registries.end();
        if (!registryOwned || registry.m_unloading)
        {
            operation.m_state = MaterializationState::kCanceled;
            operation.m_error = "Materialization owner is foreign or unloading";
        }
        operation.m_collection = collection;
        operation.m_hasDefinition = true;
        const MaterializationToken token{ m_impl->m_token, static_cast<uint32_t>(m_impl->m_materializations.size()) };
        m_impl->m_materializations.push_back(Memory::DefaultNew<Impl::Materialization>(std::move(operation)));
        return token;
    }


    MaterializationToken EntityWorld::SpawnCollection(EntityRegistry& registry, IO::AssetID collection)
    {
        auto token = SpawnCollection(registry, EntityCollection{});
        auto& operation = *m_impl->m_materializations[token.m_index];
        operation.m_hasDefinition = false;
        operation.m_asset = collection;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollection>(collection));
        return token;
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, IO::AssetID placement)
    {
        FE_Assert(Threading::IsMainThread());
        for (uint32_t index = 0; index < m_impl->m_materializations.size(); ++index)
        {
            const auto& operation = *m_impl->m_materializations[index];
            if (operation.m_placement && operation.m_asset == placement && operation.m_state <= MaterializationState::kReady)
                return { m_impl->m_token, index };
        }
        auto token = SpawnCollection(registry, EntityCollection{});
        auto& operation = *m_impl->m_materializations[token.m_index];
        operation.m_placement = true;
        operation.m_hasDefinition = false;
        operation.m_asset = placement;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollectionInstanceAsset>(placement));
        return token;
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, IO::AssetID placement,
                                                    const EntityCollectionInstanceAsset& definition,
                                                    const EntityCollection& collection)
    {
        FE_Assert(Threading::IsMainThread());
        for (uint32_t index = 0; index < m_impl->m_materializations.size(); ++index)
        {
            const auto& operation = *m_impl->m_materializations[index];
            if (operation.m_placement && operation.m_asset == placement && operation.m_state <= MaterializationState::kReady)
                return { m_impl->m_token, index };
        }
        auto token = SpawnCollection(registry, collection);
        auto& operation = *m_impl->m_materializations[token.m_index];
        operation.m_placement = true;
        operation.m_asset = placement;
        operation.m_definition = definition;
        return token;
    }


    MaterializationStatus EntityWorld::GetMaterializationStatus(MaterializationToken token) const
    {
        if (token.m_world != m_impl->m_token || token.m_index >= m_impl->m_materializations.size())
            return {};

        const auto& operation = *m_impl->m_materializations[token.m_index];
        return { operation.m_state, operation.m_root, operation.m_error };
    }


    festd::span<const EntityUuidBinding> EntityWorld::GetMaterializationBindings(MaterializationToken token) const
    {
        if (token.m_world != m_impl->m_token || token.m_index >= m_impl->m_materializations.size())
            return {};
        return m_impl->m_materializations[token.m_index]->m_bindings;
    }


    void EntityWorld::CancelMaterialization(MaterializationToken token)
    {
        FE_Assert(Threading::IsMainThread());
        FE_Assert(!m_impl->m_collecting && !m_impl->m_executing);
        if (token.m_world != m_impl->m_token || token.m_index >= m_impl->m_materializations.size())
            return;

        auto& operation = *m_impl->m_materializations[token.m_index];
        if (operation.m_state == MaterializationState::kCanceled)
            return;

        operation.m_state = MaterializationState::kCanceled;
        operation.m_request.Reset();
        operation.m_collectionRequest.Reset();
        for (const auto id : operation.m_entities)
        {
            if (auto* entity = Find(id))
                DestroyEntity(*entity);
        }
        operation.m_entities.clear();
        operation.m_collection = {};
        operation.m_definition = {};
        operation.m_root = {};
    }


    bool EntityWorld::Materialize(uint32_t index)
    {
        auto& operation = *m_impl->m_materializations[index];
        const auto& collection = operation.m_collection;

        auto reject = [&](festd::ascii_view error) {
            operation.m_error = error;
            operation.m_state = MaterializationState::kFailed;
            return false;
        };

        if (!collection.Validate())
            return reject("Invalid collection hierarchy or component envelope");
        if (operation.m_placement && (!operation.m_asset.IsValid() || !operation.m_definition.Validate(collection)))
            return reject("Invalid placement UUID bindings or root definition");

        operation.m_bindings = operation.m_placement ? operation.m_definition.m_bindings : festd::vector<EntityUuidBinding>{};
        if (!operation.m_placement)
        {
            for (const auto& entity : collection.m_entities)
                operation.m_bindings.push_back({ entity.m_uuid, NewEntityUuid() });
        }
        const Uuid rootUuid = operation.m_placement ? operation.m_definition.m_rootUuid : NewEntityUuid();
        if (Find(rootUuid, false))
            return reject("Placement root UUID is already owned");

        for (const auto& binding : operation.m_bindings)
        {
            if (Find(binding.m_entityUuid, false))
                return reject("Concrete entity UUID is already owned");
        }
        festd::vector<const EntityRecord*> records;
        festd::vector<const EntityCollection*> definitions;
        EntityRecord emptyRoot;
        emptyRoot.m_uuid = rootUuid;
        records.push_back(operation.m_placement ? &operation.m_definition.m_root.m_entities.front() : &emptyRoot);
        definitions.push_back(operation.m_placement ? &operation.m_definition.m_root : &collection);
        for (const auto& entity : collection.m_entities)
        {
            records.push_back(&entity);
            definitions.push_back(&collection);
        }

        festd::vector<festd::vector<const EntityComponentInfo*>> schemas;
        for (const auto* record : records)
        {
            festd::vector<const EntityComponentInfo*> columns;
            for (const auto& component : record->m_components)
            {
                const auto* info = Components().Find(component.m_type);
                if (!info || info->m_policy.m_transient || !info->m_type->m_deserialize || !info->m_type->m_defaultConstructor)
                    return reject("Unknown, transient or non-deserializable component type");
                if (component.m_version != info->m_type->m_serializationVersion
                    || component.m_schemaHash != info->m_type->m_serializationSchemaHash)
                {
                    return reject("Unsupported component schema or version");
                }
                columns.push_back(info);
            }
            for (uint32_t column = 0; column < columns.size(); ++column)
            {
                for (const auto companion : columns[column]->m_runtimeCompanions)
                {
                    const auto* info = Components().Find(companion);
                    if (!info || !info->m_policy.m_transient || !info->m_type->m_defaultConstructor)
                        return reject("Invalid runtime companion registration");
                    if (festd::find(columns, info) == columns.end())
                        columns.push_back(info);
                }
            }
            festd::sort(columns.begin(), columns.end(), [](const auto* a, const auto* b) {
                return a->m_type->m_id < b->m_type->m_id;
            });
            Archetype validation(columns);
            if (validation.m_lifecycleOrder.size() != columns.size())
                return reject("Missing or cyclic component initialization dependency");

            schemas.push_back(std::move(columns));
        }

        auto remap = [](void* data, Uuid uuid) {
            auto& bindings = *static_cast<festd::vector<EntityUuidBinding>*>(data);
            for (const auto& binding : bindings)
            {
                if (binding.m_sourceUuid == uuid)
                    return binding.m_entityUuid;
            }
            return uuid;
        };
        bool success = false;
        auto rollback = festd::defer([&] {
            if (success)
                return;

            for (const auto id : operation.m_entities)
            {
                if (auto* entity = Find(id))
                    DestroyEntity(*entity);
            }
            operation.m_entities.clear();
            operation.m_root = {};
        });

        for (uint32_t row = 0; row < records.size(); ++row)
        {
            const auto& record = *records[row];
            Entity* entity = AllocateEntity(*operation.m_registry,
                                            Env::Name(record.m_name),
                                            row == 0 ? rootUuid : remap(&operation.m_bindings, record.m_uuid));
            entity->m_wantsActive = false;
            festd::vector<void*> values(schemas[row].size(), nullptr);
            Migrate(*entity, schemas[row], values);
            operation.m_entities.push_back(entity->m_id);
            for (const auto& component : record.m_components)
            {
                const auto* info = Components().Find(component.m_type);
                const auto& definition = *definitions[row];
                IO::ReadOnlyMemoryStream stream(definition.m_payload.data() + component.m_payloadOffset, component.m_payloadSize);
                Serialization::PackedBinaryFormat format;
                Serialization::DeserializationContext context(&stream, format);
                context.SetObjectReferenceRemapper(&operation.m_bindings, remap);
                void* destination = entity->FindComponent(component.m_type);
                if (context.Load(*info->m_type, destination) != Serialization::ResultCode::kSuccess
                    || stream.Tell() != stream.Length())
                {
                    return reject("Corrupt component payload");
                }
                // Envelope ownership remains independent of the definition's acquisition and generation pin.
                m_impl->m_loadingComponent = component.m_type;
                for (const auto& dependency : component.m_dependencies)
                {
                    if (dependency.m_kind == IO::DependencyKind::kHard)
                        RequireAsset(*entity, dependency.m_asset, dependency.m_expectedType);
                }
                m_impl->m_loadingComponent = Rtti::TypeID::kNull;
            }
        }

        operation.m_root = operation.m_entities.front();
        for (uint32_t row = 1; row < records.size(); ++row)
        {
            const Uuid parent = records[row]->m_parentUuid;
            Entity* parentEntity = parent.IsValid() ? Find(remap(&operation.m_bindings, parent), false) : Find(operation.m_root);
            Reparent(*Find(operation.m_entities[row]), parentEntity);
        }

        for (const auto id : operation.m_entities)
            Find(id)->m_wantsActive = true;
        for (const auto id : operation.m_entities)
        {
            Entity& entity = *Find(id);
            for (uint32_t column = 0; column < entity.m_chunk->m_archetype.m_columns.size(); ++column)
            {
                const auto& info = *entity.m_chunk->m_archetype.m_columns[column];
                auto& stage = entity.m_chunk->Stage(entity.m_row, column);
                if (LoadValue(entity, info, entity.m_chunk->Get(entity.m_row, column), stage) == LifecycleResult::kFailed)
                    return reject("Materialized dependency discovery failed");
            }
        }

        success = true;
        return true;
    }


    void EntityWorld::AdvanceMaterializations(bool bootstrap)
    {
        const uint32_t operationCount = static_cast<uint32_t>(m_impl->m_materializations.size());
        for (uint32_t index = 0; index < operationCount; ++index)
        {
            auto& operation = *m_impl->m_materializations[index];
            if (operation.m_state == MaterializationState::kReady && !Find(operation.m_root))
                CancelMaterialization({ m_impl->m_token, index });
            if (operation.m_state != MaterializationState::kPending
                || (!bootstrap && operation.m_eligibleEpoch > m_impl->m_epoch))
            {
                continue;
            }
            const bool registryPresent = festd::find(m_impl->m_registries, operation.m_registry) != m_impl->m_registries.end();
            if (!registryPresent || operation.m_registry->GetID() != operation.m_registryId)
            {
                CancelMaterialization({ m_impl->m_token, index });
                continue;
            }
            auto fail = [&](festd::ascii_view error) {
                CancelMaterialization({ m_impl->m_token, index });
                operation.m_state = MaterializationState::kFailed;
                operation.m_error = error;
            };
            if (operation.m_entities.empty() && !operation.m_hasDefinition)
            {
                if (!operation.m_request.IsValid())
                {
                    fail("Definition asset request unavailable");
                    continue;
                }
                if (!operation.m_request.IsCompleted())
                    continue;
                if (operation.m_request.GetResult() != IO::AssetLoadResult::kSucceeded)
                {
                    fail("Definition asset load failed");
                    continue;
                }
                if (operation.m_placement)
                {
                    if (!operation.m_collectionRequest.IsValid())
                    {
                        auto read = IO::AssetHandle<EntityCollectionInstanceAsset>(operation.m_request.GetAssetSlot()).Read();
                        if (!read.Get())
                        {
                            fail("Placement definition generation unavailable");
                            continue;
                        }
                        operation.m_definition = *read.Get();
                        operation.m_collectionRequest = IO::AssetManager::LoadAsset(operation.m_definition.m_collection);
                    }
                    if (!operation.m_collectionRequest.IsCompleted())
                        continue;
                    if (operation.m_collectionRequest.GetResult() != IO::AssetLoadResult::kSucceeded)
                    {
                        fail("Collection definition load failed");
                        continue;
                    }
                    auto read = IO::AssetHandle<EntityCollection>(operation.m_collectionRequest.GetAssetSlot()).Read();
                    if (!read.Get())
                    {
                        fail("Collection definition generation unavailable");
                        continue;
                    }
                    operation.m_collection = *read.Get();
                }
                else
                {
                    auto read = IO::AssetHandle<EntityCollection>(operation.m_request.GetAssetSlot()).Read();
                    if (!read.Get())
                    {
                        fail("Collection definition generation unavailable");
                        continue;
                    }
                    operation.m_collection = *read.Get();
                }
                operation.m_hasDefinition = true;
            }
            if (operation.m_entities.empty())
            {
                if (!Materialize(index))
                {
                    operation.m_request.Reset();
                    operation.m_collectionRequest.Reset();
                    operation.m_collection = {};
                    operation.m_definition = {};
                    continue;
                }
                // Runtime contributions have been acquired before releasing definition requests.
                operation.m_request.Reset();
                operation.m_collectionRequest.Reset();
                operation.m_collection = {};
                operation.m_definition = {};
            }
        }
        AdvanceLifecycle();
        for (uint32_t index = 0; index < m_impl->m_materializations.size(); ++index)
        {
            auto& operation = *m_impl->m_materializations[index];
            if (operation.m_state != MaterializationState::kPending || operation.m_entities.empty())
                continue;

            bool failed = false;
            bool ready = true;
            for (const auto id : operation.m_entities)
            {
                const auto* entity = Find(id);
                failed |= !entity || entity->HasFailed();
                ready &= entity && entity->IsActive();
            }
            if (failed)
            {
                CancelMaterialization({ m_impl->m_token, index });
                operation.m_state = MaterializationState::kFailed;
                operation.m_error = "Materialized component lifecycle failed";
            }
            else if (ready)
                operation.m_state = MaterializationState::kReady;
        }
    }
} // namespace FE::Framework
