#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldInternal.h>
#include <algorithm>

namespace FE::Framework
{
    namespace
    {
        bool DestructionContains(const EntityWorld& world, const Command& command, const EntityID target)
        {
            if (command.m_kind != CommandKind::kDestroy)
                return false;

            for (const Entity* current = world.Find(target); current; current = current->GetParent())
            {
                if (current->GetID() == command.GetTarget().m_id)
                    return true;
            }

            return false;
        }


        const EntityRegistry* GetCommandRegistry(const EntityWorld& world, const Command& command)
        {
            if (EntityRegistry* registry = command.GetRegistry())
                return registry;

            const Entity* entity = world.Find(command.GetTarget().m_id);
            return entity ? &entity->GetRegistry() : nullptr;
        }


        bool CommandsConflict(const EntityWorld& world, const Command& first, const Command& second)
        {
            if (first.GetTarget().m_id.m_value && first.GetTarget().m_id == second.GetTarget().m_id)
                return true;

            if (DestructionContains(world, first, second.GetTarget().m_id)
                || DestructionContains(world, second, first.GetTarget().m_id))
            {
                return true;
            }

            if (DestructionContains(world, first, second.GetParent().m_id)
                || DestructionContains(world, second, first.GetParent().m_id))
            {
                return true;
            }

            if (first.m_kind == CommandKind::kParent && second.m_kind == CommandKind::kParent)
            {
                if (first.GetParent().m_id.m_value && first.GetParent().m_id == second.GetTarget().m_id)
                    return true;

                if (second.GetParent().m_id.m_value && second.GetParent().m_id == first.GetTarget().m_id)
                    return true;
            }

            if (first.m_kind != CommandKind::kUnloadRegistry && second.m_kind != CommandKind::kUnloadRegistry)
                return false;

            const EntityRegistry* firstRegistry = GetCommandRegistry(world, first);
            return firstRegistry && firstRegistry == GetCommandRegistry(world, second);
        }


        bool CommandListsConflict(const EntityWorld& world, const festd::span<const Command> first,
                                  const festd::span<const Command> second)
        {
            for (const auto& firstCommand : first)
            {
                for (const auto& secondCommand : second)
                {
                    if (CommandsConflict(world, firstCommand, secondCommand))
                        return true;
                }
            }

            return false;
        }
    } // namespace


    void EntityWorld::Impl::Submit(EntityCommandList&& commands)
    {
        FE_PROFILER_ZONE();

        FE_AssertDebug(commands.m_impl && commands.m_impl->m_world == &m_owner);

        auto& list = *commands.m_impl;
        list.m_eligibleEpoch = m_schedule.m_epoch;

        bool hasPublication = false;
        bool hasHierarchyEdits = false;
        for (const auto& command : list.m_commands)
        {
            hasPublication |= command.m_kind == CommandKind::kCreate || command.m_kind == CommandKind::kComponent;
            hasHierarchyEdits |= command.m_kind == CommandKind::kParent;
        }

        // Preserve detach/destroy recording order when a hierarchy batch also needs deferred publication.
        if (!hasPublication || hasHierarchyEdits)
        {
            if (hasPublication)
                ++list.m_eligibleEpoch;

            std::lock_guard lock{ m_pendingCommands.m_lock };
            m_pendingCommands.m_lists.push_back(std::exchange(commands.m_impl, nullptr));
            return;
        }

        festd::inline_vector<EntityID> deferredTargets;
        festd::inline_vector<EntityID> destroyedTargets;
        for (const auto& command : list.m_commands)
        {
            if (command.m_kind == CommandKind::kComponent && command.GetTarget().m_id.m_value)
                deferredTargets.push_back(command.GetTarget().m_id);
            if (command.m_kind == CommandKind::kDestroy && command.GetTarget().m_id.m_value)
                destroyedTargets.push_back(command.GetTarget().m_id);
        }

        // Independent existing targets can commit without waiting for creations.
        auto* immediate = Memory::DefaultNew<EntityCommandList::Impl>(m_owner, list.m_id);
        immediate->m_eligibleEpoch = m_schedule.m_epoch;

        festd::inline_vector<Command> deferred;
        for (const auto& command : list.m_commands)
        {
            const bool destroyed = festd::find(destroyedTargets, command.GetTarget().m_id) != destroyedTargets.end();

            const bool needsPublication =
                !destroyed && festd::find(deferredTargets, command.GetTarget().m_id) != deferredTargets.end();

            const bool tokenCommand = command.GetTarget().m_token.m_list || command.GetParent().m_token.m_list;
            const bool isImmediate = command.GetTarget().m_id.m_value && !tokenCommand && !needsPublication;

            if (destroyed && command.m_kind == CommandKind::kComponent)
            {
                command.m_payload.m_component.m_info->m_type->m_destructor(command.m_payload.m_component.m_value);
                continue;
            }

            if (isImmediate)
                immediate->m_commands.push_back(command);
            else
                deferred.push_back(command);
        }

        // Registry ID reservations follow their records; splitting transfers ownership without retaining again.
        list.m_commands = std::move(deferred);
        list.m_eligibleEpoch = m_schedule.m_epoch + 1;
        if (immediate->m_commands.empty())
        {
            Memory::DefaultDelete(immediate);
            immediate = nullptr;
        }

        std::lock_guard lock{ m_pendingCommands.m_lock };
        if (immediate)
            m_pendingCommands.m_lists.push_back(immediate);

        m_pendingCommands.m_lists.push_back(std::exchange(commands.m_impl, nullptr));
    }


    struct EntityWorld::Impl::Transaction
    {
        Transaction(Impl& world, EntityCommandList::Impl& list)
            : m_world(world)
            , m_list(list)
            , m_tokens(list.m_created, kInvalidIndex)
        {
        }


        bool Execute()
        {
            FE_PROFILER_ZONE();
            if (!Record() || !Validate())
                return false;

            Publish();
            return true;
        }

    private:
        struct Value
        {
            const EntityComponentInfo* m_info;
            void* m_value;
        };


        struct Edit
        {
            Entity* m_entity = nullptr;
            EntityRegistry* m_registry = nullptr;
            Uuid m_uuid = Uuid::kNull;
            Env::Name m_name;
            uint32_t m_parent = kInvalidIndex;
            ResidencyScope m_residency = ResidencyScope::kEntity;
            bool m_created = false;
            bool m_destroy = false;
            bool m_unload = false;
            bool m_wantsActive = true;
            bool m_touched = false;
            bool m_componentsChanged = false;
            bool m_parentChanged = false;
            festd::vector<Value> m_values;
            bool m_hasValues = false;
            Archetype* m_archetype = nullptr;

            void PrepareValues()
            {
                if (m_hasValues)
                    return;

                m_hasValues = true;
                if (!m_entity)
                    return;

                for (const auto* info : m_entity->m_chunk->m_archetype.m_columns)
                    m_values.push_back({ info, nullptr });
            }
        };


        bool Record()
        {
            auto& list = m_list;
            festd::unordered_dense_map<Entity*, uint32_t> entityIndices;
            auto include = [&](Entity* entity) {
                if (!entity)
                    return kInvalidIndex;

                const auto found = entityIndices.find(entity);
                if (found != entityIndices.end())
                    return found->second;

                Edit edit;
                edit.m_entity = entity;
                edit.m_registry = entity->m_registry;
                edit.m_uuid = entity->m_uuid;
                edit.m_name = entity->m_name;
                edit.m_wantsActive = entity->m_wantsActive;
                const uint32_t index = m_edits.size();
                m_edits.push_back(std::move(edit));
                entityIndices.emplace(entity, index);
                return index;
            };

            auto includeAncestors = [&](Entity* entity) {
                for (Entity* current = entity; current; current = current->m_parent)
                    include(current);
            };
            auto includeSubtree = [&](Entity* root) {
                if (!root)
                    return;

                for (Entity* entity = root; entity;)
                {
                    include(entity);
                    if (entity->m_firstChild)
                    {
                        entity = entity->m_firstChild;
                        continue;
                    }

                    while (entity != root && !entity->m_nextSibling)
                        entity = entity->m_parent;

                    entity = entity == root ? nullptr : entity->m_nextSibling.Get();
                }
            };

            const bool destroys = std::any_of(list.m_commands.begin(), list.m_commands.end(), [](const Command& command) {
                return command.m_kind == CommandKind::kDestroy;
            });
            const bool topology =
                destroys || std::any_of(list.m_commands.begin(), list.m_commands.end(), [](const Command& command) {
                    return command.m_kind == CommandKind::kParent;
                });
            for (const auto& command : list.m_commands)
            {
                Entity* target = m_world.m_owner.Find(command.GetTarget().m_id);
                include(target);
                if (topology)
                {
                    includeAncestors(target);
                    includeAncestors(m_world.m_owner.Find(command.GetParent().m_id));
                    if (command.m_kind == CommandKind::kDestroy || (destroys && command.m_kind == CommandKind::kParent))
                        includeSubtree(target);
                }
            }
            if (topology)
            {
                for (auto& edit : m_edits)
                {
                    const auto parent = entityIndices.find(edit.m_entity->m_parent.Get());
                    edit.m_parent = parent == entityIndices.end() ? kInvalidIndex : parent->second;
                }
            }

            auto entityIndex = [&](Entity* entity) {
                const auto found = entityIndices.find(entity);
                return found == entityIndices.end() ? kInvalidIndex : found->second;
            };

            auto resolve = [&](const EntityTarget& target) {
                if (target.m_token.m_list)
                {
                    if (target.m_token.m_list != list.m_id || target.m_token.m_index >= m_tokens.size())
                        return kInvalidIndex;

                    return m_tokens[target.m_token.m_index];
                }
                return entityIndex(m_world.m_owner.Find(target.m_id));
            };

            struct EditContext
            {
                festd::vector<Edit>* m_edits;
                EntityCommandList::Impl* m_list;
            } editContext{ &m_edits, &list };

            auto component = [](void* data, const uint32_t index, Rtti::TypeID type, const bool write) -> void* {
                auto& context = *static_cast<EditContext*>(data);
                if (index >= context.m_edits->size())
                    return nullptr;

                auto& edit = (*context.m_edits)[index];
                edit.PrepareValues();

                auto value = festd::find_if(edit.m_values.begin(), edit.m_values.end(), [&](const Value& item) {
                    return item.m_info->m_type->m_id == type;
                });
                if (value == edit.m_values.end())
                    return nullptr;

                if (value->m_value)
                    return value->m_value;

                void* source = edit.m_entity ? edit.m_entity->FindComponent(type) : nullptr;
                if (!write)
                    return source;

                const auto& rtti = *value->m_info->m_type;
                if (!source || !rtti.m_copyConstructor)
                    return nullptr;

                void* storage = context.m_list->m_arena.allocate(rtti.m_size, rtti.m_alignment);
                if (!storage)
                {
                    storage = Memory::DefaultAllocate(rtti.m_size, rtti.m_alignment);
                    context.m_list->m_largePayloads.push_back(storage);
                }

                rtti.m_copyConstructor(storage, source);
                context.m_list->m_ownedValues.push_back({ value->m_info, storage });
                value->m_value = storage;
                edit.m_componentsChanged = true;
                return storage;
            };

            // Build an isolated edit snapshot first; rejected topology or content cannot mutate live rows.
            bool valid = true;
            for (const auto& command : list.m_commands)
            {
                if (command.m_kind == CommandKind::kCreate)
                {
                    const auto& creation = command.m_payload.m_create;
                    if (festd::find(m_world.m_storage.m_registries, creation.m_registry) == m_world.m_storage.m_registries.end())
                    {
                        valid = m_world.Fail("Invalid registry");
                        break;
                    }

                    if (creation.m_registry->GetID() != creation.m_registryId)
                    {
                        valid = m_world.Fail("Invalid registry");
                        break;
                    }

                    if (m_world.m_owner.Find(creation.m_uuid, false))
                        valid = m_world.Fail("Duplicate entity UUID");

                    for (const auto& edit : m_edits)
                    {
                        if (edit.m_created && edit.m_uuid == creation.m_uuid)
                            valid = m_world.Fail("Duplicate entity UUID");
                    }

                    Edit edit;
                    edit.m_registry = creation.m_registry;
                    edit.m_uuid = creation.m_uuid;
                    edit.m_name = creation.m_name;
                    edit.m_residency = creation.m_residency;
                    edit.m_created = edit.m_touched = true;
                    m_tokens[command.GetTarget().m_token.m_index] = m_edits.size();

                    m_edits.push_back(std::move(edit));
                    continue;
                }

                if (command.m_kind == CommandKind::kUnloadRegistry)
                {
                    const auto& unload = command.m_payload.m_registry;
                    if (festd::find(m_world.m_storage.m_registries, unload.m_registry) == m_world.m_storage.m_registries.end())
                        valid = m_world.Fail("Invalid registry");
                    else if (unload.m_registry->GetID() != unload.m_id)
                        valid = m_world.Fail("Invalid registry");
                    else
                        m_unloadRegistries.push_back(unload.m_registry);

                    continue;
                }

                const uint32_t index = resolve(command.GetTarget());
                if (index == kInvalidIndex)
                {
                    valid = m_world.Fail("Stale entity ID or invalid list-local token");
                    break;
                }

                auto& edit = m_edits[index];
                if (edit.m_destroy)
                    continue;

                edit.m_touched = true;
                switch (command.m_kind)
                {
                case CommandKind::kDestroy:
                    {
                        // Resolve descendants through the overlay, including earlier detach/reparent commands.
                        festd::vector<festd::inline_vector<uint32_t>> children(m_edits.size());
                        for (uint32_t child = 0; child < m_edits.size(); ++child)
                        {
                            const uint32_t parent = m_edits[child].m_parent;
                            if (parent != kInvalidIndex)
                                children[parent].push_back(child);
                        }

                        festd::vector<bool> visited(m_edits.size(), false);
                        festd::inline_vector<uint32_t> pending{ index };
                        while (!pending.empty())
                        {
                            const uint32_t target = pending.back();
                            pending.pop_back();
                            if (visited[target])
                                continue;

                            visited[target] = true;
                            m_edits[target].m_destroy = true;
                            m_edits[target].m_touched = true;
                            for (uint32_t child : children[target])
                                pending.push_back(child);
                        }
                        break;
                    }

                case CommandKind::kRename:
                    edit.m_name = command.m_payload.m_name;
                    break;

                case CommandKind::kActive:
                    edit.m_wantsActive = command.m_payload.m_active;
                    break;

                case CommandKind::kUnload:
                    edit.m_unload = true;
                    edit.m_wantsActive = false;
                    break;

                case CommandKind::kParent:
                    {
                        const bool nullParent = !command.GetParent().m_id.m_value && !command.GetParent().m_token.m_list;
                        const uint32_t parent = nullParent ? kInvalidIndex : resolve(command.GetParent());

                        if (!nullParent && parent == kInvalidIndex)
                        {
                            valid = m_world.Fail("Invalid parent handle");
                        }
                        else if (parent != kInvalidIndex && m_edits[parent].m_registry != edit.m_registry)
                        {
                            valid = m_world.Fail("Parenting cannot cross registry or world boundaries");
                        }
                        else
                        {
                            if (edit.m_parent != parent && m_world.m_reparentHandler
                                && command.m_payload.m_parent.m_mode == ReparentMode::kPreserveWorld)
                            {
                                ReparentContext context{ index,
                                                         parent,
                                                         m_edits.size(),
                                                         command.m_payload.m_parent.m_mode,
                                                         &editContext,
                                                         [](void* data, uint32_t target) {
                                                             return (*static_cast<EditContext*>(data)->m_edits)[target].m_parent;
                                                         },
                                                         component };
                                if (!m_world.m_reparentHandler(context))
                                {
                                    valid = m_world.Fail("Invalid reparent transform");
                                    break;
                                }
                            }

                            edit.m_parent = parent;
                            edit.m_parentChanged = true;
                        }
                        break;
                    }

                case CommandKind::kComponent:
                case CommandKind::kRemove:
                    {
                        edit.PrepareValues();
                        const auto type = command.m_kind == CommandKind::kComponent
                            ? command.m_payload.m_component.m_info->m_type->m_id
                            : command.m_payload.m_remove;

                        const auto found = festd::find_if(edit.m_values.begin(), edit.m_values.end(), [&](const Value& value) {
                            return value.m_info->m_type->m_id == type;
                        });
                        if (found != edit.m_values.end())
                        {
                            const auto companions = found->m_info->m_runtimeCompanions;
                            edit.m_values.erase(found);
                            if (command.m_kind == CommandKind::kRemove)
                            {
                                for (const auto companion : companions)
                                {
                                    const bool stillRequired =
                                        std::any_of(edit.m_values.begin(), edit.m_values.end(), [companion](const Value& value) {
                                            const auto& owned = value.m_info->m_runtimeCompanions;
                                            return festd::find(owned, companion) != owned.end();
                                        });
                                    if (stillRequired)
                                        continue;

                                    const auto runtime = festd::find_if(edit.m_values.begin(),
                                                                        edit.m_values.end(),
                                                                        [companion](const Value& value) {
                                                                            return value.m_info->m_type->m_id == companion;
                                                                        });
                                    if (runtime != edit.m_values.end())
                                        edit.m_values.erase(runtime);
                                }
                            }
                        }

                        if (command.m_kind == CommandKind::kComponent)
                        {
                            edit.m_values.push_back(
                                { command.m_payload.m_component.m_info, command.m_payload.m_component.m_value });
                            // Add companions with their authored component, preserving later explicit removals in command order.
                            for (const auto companion : command.m_payload.m_component.m_info->m_runtimeCompanions)
                            {
                                const auto runtime =
                                    festd::find_if(edit.m_values.begin(), edit.m_values.end(), [companion](const Value& value) {
                                        return value.m_info->m_type->m_id == companion;
                                    });
                                if (runtime == edit.m_values.end())
                                    edit.m_values.push_back({ m_world.m_components.Find(companion), nullptr });
                            }
                        }
                        edit.m_componentsChanged = true;
                        break;
                    }
                default:
                    break;
                }

                if (!valid)
                    break;
            }

            return valid;
        }


        bool Validate()
        {
            // A three-state walk validates each overlay edge once, even for a long ancestor chain.
            enum class Visit : uint8_t
            {
                kUnvisited,
                kVisiting,
                kComplete
            };
            festd::vector<Visit> visits(m_edits.size(), Visit::kUnvisited);
            festd::inline_vector<uint32_t> path;
            for (uint32_t i = 0; i < m_edits.size(); ++i)
            {
                path.clear();
                uint32_t current = i;
                while (current != kInvalidIndex && visits[current] == Visit::kUnvisited)
                {
                    visits[current] = Visit::kVisiting;
                    path.push_back(current);
                    current = m_edits[current].m_parent;
                }
                if (current != kInvalidIndex && visits[current] == Visit::kVisiting)
                    return m_world.Fail("Invalid entity hierarchy");

                for (uint32_t node : path)
                    visits[node] = Visit::kComplete;
            }

            bool valid = true;
            for (uint32_t i = 0; valid && i < m_edits.size(); ++i)
            {
                const uint32_t parent = m_edits[i].m_parent;
                if (parent != kInvalidIndex && m_edits[parent].m_destroy && !m_edits[i].m_destroy)
                    return m_world.Fail("Cannot parent an entity to a target queued for destruction");

                auto& edit = m_edits[i];
                if (!edit.m_touched || edit.m_destroy)
                    continue;

                if (festd::find(m_unloadRegistries, edit.m_registry) != m_unloadRegistries.end())
                {
                    valid = m_world.Fail("Cannot edit and unload the same registry in one batch");
                    break;
                }

                if (!edit.m_created && !edit.m_componentsChanged)
                    continue;

                festd::sort(edit.m_values.begin(), edit.m_values.end(), [](const Value& a, const Value& b) {
                    return a.m_info->m_type->m_id < b.m_info->m_type->m_id;
                });

                festd::vector<const EntityComponentInfo*> columns;
                for (const auto& value : edit.m_values)
                    columns.push_back(value.m_info);

                edit.m_archetype = m_world.GetArchetype(columns);
            }

            return valid;
        }


        void Publish()
        {
            // No entity/link/storage mutation precedes validation of the complete list.
            for (auto& edit : m_edits)
            {
                if (edit.m_created && !edit.m_destroy)
                {
                    edit.m_entity = m_world.AllocateEntity(*edit.m_registry, edit.m_name, edit.m_uuid);
                    edit.m_entity->m_residencyScope = edit.m_residency;
                }
            }

            for (auto& edit : m_edits)
            {
                if (!edit.m_touched || !edit.m_entity || edit.m_destroy)
                    continue;

                Entity& entity = *edit.m_entity;
                entity.m_name = edit.m_name;
                if (edit.m_created || edit.m_componentsChanged)
                {
                    festd::inline_vector<void*> values;
                    for (const auto& value : edit.m_values)
                        values.push_back(value.m_value);

                    m_world.Migrate(entity, *edit.m_archetype, values);
                    entity.m_failed = false;
                }

                if (entity.m_wantsActive != edit.m_wantsActive)
                    m_world.m_lifecycle.MarkUnready(entity);

                entity.m_wantsActive = edit.m_wantsActive;
            }

            // Detach changed links first so a valid final topology cannot temporarily form a cycle.
            for (auto& edit : m_edits)
            {
                if (edit.m_parentChanged && edit.m_entity)
                    m_world.Reparent(*edit.m_entity, nullptr);
            }

            for (auto& edit : m_edits)
            {
                if (edit.m_parentChanged && edit.m_entity && edit.m_parent != kInvalidIndex)
                    m_world.Reparent(*edit.m_entity, m_edits[edit.m_parent].m_entity);
            }

            for (auto& edit : m_edits)
            {
                Entity* entity = m_world.m_owner.Find(edit.m_uuid, false);
                if (!entity || !edit.m_touched)
                    continue;

                if (edit.m_destroy)
                {
                    m_world.DestroyEntity(*entity);
                    continue;
                }

                if (!edit.m_wantsActive)
                    m_world.m_lifecycle.DeactivateSubtree(*entity);

                if (edit.m_unload)
                {
                    // Unload keeps allocated identity but resets runtime state, descendants first.
                    m_world.m_lifecycle.UnloadSubtree(*entity);
                }
            }

            for (auto* registry : m_unloadRegistries)
                m_world.m_owner.RemoveRegistry(*registry);
        }

        Impl& m_world;
        EntityCommandList::Impl& m_list;
        festd::vector<Edit> m_edits;
        festd::vector<uint32_t> m_tokens;
        festd::vector<EntityRegistry*> m_unloadRegistries;
    };


    bool EntityWorld::Impl::CommitImpl(const bool bootstrap)
    {
        FE_PROFILER_ZONE();

        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_AssertDebug(!m_schedule.IsCollecting() && !m_schedule.IsExecuting(), "Structural commit requires a safe boundary");

        m_error = {};

        // Detach eligible batches under the submission lock; validation and callbacks run after releasing it.
        festd::vector<EntityCommandList::Impl*> ready;
        {
            std::lock_guard lock{ m_pendingCommands.m_lock };
            for (auto it = m_pendingCommands.m_lists.begin(); it != m_pendingCommands.m_lists.end();)
            {
                if (!bootstrap && (*it)->m_eligibleEpoch > m_schedule.m_epoch)
                {
                    ++it;
                    continue;
                }
                ready.push_back(*it);
                it = m_pendingCommands.m_lists.erase(it);
            }
        }

        auto cleanup = festd::defer([&] {
            for (auto* list : ready)
                Memory::DefaultDelete(list);
        });

        festd::vector<bool> conflicts(ready.size(), false);
        for (uint32_t first = 0; first < ready.size(); ++first)
        {
            for (uint32_t second = first + 1; second < ready.size(); ++second)
            {
                if (ready[first]->m_id == ready[second]->m_id)
                    continue;

                if (CommandListsConflict(m_owner, ready[first]->m_commands, ready[second]->m_commands))
                    conflicts[first] = conflicts[second] = true;
            }
        }

        bool success = true;
        for (uint32_t listIndex = 0; listIndex < ready.size(); ++listIndex)
        {
            auto& list = *ready[listIndex];
            if (conflicts[listIndex])
            {
                success = Fail("Independent command lists conflict on the same target");
                continue;
            }

            Transaction transaction(*this, list);
            success &= transaction.Execute();
        }

        m_materializer.Advance(bootstrap);
        m_lifecycle.Advance();
        m_materializer.UpdateStatus(bootstrap);
        return success;
    }
} // namespace FE::Framework
