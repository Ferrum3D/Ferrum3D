#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntitySerialization.h>
#include <Framework/Entities/EntityWorldInternal.h>

namespace FE::Framework
{
    bool EntityWorld::Impl::LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations)
    {
        FE_PROFILER_ZONE();
        FE_AssertDebug(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());
        FE_Assert(m_storage.m_registries.empty(), "Definitions require an empty world");

        if (!definition.Validate())
            return false;

        for (const auto& group : definition.m_registries)
        {
            EntityRegistry& registry = CreateRegistry(group.m_key);
            const auto concrete = m_materializer.LoadEntities(registry, group.m_entities, true);
            if (operations)
                operations->push_back(concrete);

            for (const auto& placement : group.m_placements)
            {
                const auto token = m_materializer.LoadPlacement(registry, placement.GetAssetID());
                if (operations)
                    operations->push_back(token);
            }
        }

        return true;
    }


    bool EntityWorld::Impl::CaptureSnapshot(EntityWorldSnapshotAsset& snapshot)
    {
        FE_PROFILER_ZONE();
        FE_AssertDebug(Threading::IsMainThread());
        FE_Assert(!m_schedule.IsUpdating(), "Snapshot capture requires a closed update epoch");

        std::lock_guard lock{ m_pendingCommands.m_lock };
        if (!m_pendingCommands.m_lists.empty())
            return Fail("Snapshot blocked by pending entity commands");

        if (m_materializer.HasPending())
            return Fail("Snapshot blocked by pending entity materialization");

        EntityWorldSnapshotAsset result;
        for (EntityRegistry* registry : m_storage.m_registries)
        {
            if (registry->IsTransient())
                continue;

            EntityWorldRegistrySnapshot group;
            group.m_key = registry->GetKey();

            festd::vector<const Entity*> pending;
            for (const auto& slot : m_storage.m_slots)
            {
                if (slot.m_entity && slot.m_entity->m_registry == registry && !slot.m_entity->m_parent)
                    pending.push_back(slot.m_entity);
            }

            // A preorder walk preserves sibling order while avoiding recursion for deep hierarchies.
            while (!pending.empty())
            {
                const Entity* entity = pending.back();
                pending.pop_back();
                for (const Entity* child = entity->m_lastChild; child; child = child->m_previousSibling)
                    pending.push_back(child);

                if (EntityLifecycle::HasPendingReplacements(*entity))
                    return Fail("Snapshot blocked by pending component replacements");

                bool wantsPublication = true;
                for (const Entity* ancestor = entity; ancestor; ancestor = ancestor->m_parent)
                    wantsPublication &= ancestor->m_wantsActive;

                if (wantsPublication && !entity->IsActive())
                    return Fail("Snapshot blocked by unsettled entity lifecycle");

                EntityRecord record;
                record.m_uuid = entity->m_uuid;
                record.m_parentUuid = entity->m_parent ? entity->m_parent->m_uuid : Uuid::kNull;
                record.m_name.assign(entity->m_name.c_str() ? entity->m_name.c_str() : "", entity->m_name.size());
                record.m_active = entity->m_wantsActive;
                record.m_residency = entity->m_residencyScope;
                for (uint32_t column = 0; column < entity->m_chunk->m_archetype.m_columns.size(); ++column)
                {
                    const auto& info = *entity->m_chunk->m_archetype.m_columns[column];
                    if (info.m_policy.m_transient)
                        continue;

                    Rtti::Any& component = record.m_components.emplace_back();
                    if (!info.m_type->m_serialize || !info.m_type->m_deserialize || !info.m_type->m_copyConstructor
                        || !component.Emplace(*info.m_type)
                        || !CopyComponent(*info.m_type, entity->m_chunk->Get(entity->m_row, column), component.GetValue()))
                    {
                        return Fail("Invalid serializable entity component");
                    }
                }

                group.m_entities.m_entities.push_back(std::move(record));
            }

            m_materializer.CapturePlacements(*registry, group.m_placements);
            result.m_registries.push_back(std::move(group));
        }

        snapshot = std::move(result);
        m_error = {};
        return true;
    }


    bool EntityWorld::Impl::RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot,
                                            festd::vector<MaterializationToken>* operations)
    {
        FE_PROFILER_ZONE();
        FE_AssertDebug(Threading::IsMainThread());
        FE_AssertDebug(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());
        FE_Assert(m_storage.m_registries.empty(), "Snapshots require an empty world");

        if (!snapshot.Validate())
            return false;

        for (const auto& group : snapshot.m_registries)
        {
            EntityRegistry& registry = CreateRegistry(group.m_key);

            // Materialize concrete rows now, before installing ownership records. Lifecycle may remain asynchronous.
            MaterializationToken token;
            const bool restored = m_materializer.RestoreEntities(registry, group.m_entities, token);
            if (operations)
                operations->push_back(token);

            if (!restored)
            {
                while (!m_storage.m_registries.empty())
                    RemoveRegistry(*m_storage.m_registries.back());

                return false;
            }

            for (const auto& placement : group.m_placements)
                m_materializer.RestorePlacement(registry, placement);
        }

        return true;
    }
} // namespace FE::Framework
