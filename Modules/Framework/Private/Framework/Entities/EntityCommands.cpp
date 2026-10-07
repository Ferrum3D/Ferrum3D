#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    void EntityWorld::Submit(EntityCommandList&& commands)
    {
        FE_Assert(commands.m_impl && commands.m_impl->m_world == this);
        std::lock_guard lock{ m_impl->m_commandLock };
        auto& list = *commands.m_impl;
        list.m_eligibleEpoch = m_impl->m_epoch;
        bool hasPublication = false;
        festd::vector<EntityID> deferredTargets;
        festd::vector<EntityID> destroyedTargets;
        for (const auto& command : list.m_commands)
        {
            hasPublication |= command.m_kind == CommandKind::kCreate || command.m_kind == CommandKind::kComponent;
            if (command.m_kind == CommandKind::kComponent && command.m_target.m_id.m_value)
                deferredTargets.push_back(command.m_target.m_id);
            if (command.m_kind == CommandKind::kDestroy && command.m_target.m_id.m_value)
                destroyedTargets.push_back(command.m_target.m_id);
        }
        if (!hasPublication)
        {
            m_impl->m_commands.push_back(std::exchange(commands.m_impl, nullptr));
            return;
        }
        // Keep token/hierarchy batches together. Independent existing targets can commit without waiting for creations.
        auto* immediate = Memory::DefaultNew<EntityCommandList::Impl>(*this);
        immediate->m_id = list.m_id;
        immediate->m_eligibleEpoch = m_impl->m_epoch;
        festd::vector<Command> deferred;
        for (const auto& command : list.m_commands)
        {
            const bool destroyed = festd::find(destroyedTargets, command.m_target.m_id) != destroyedTargets.end();

            const bool needsPublication =
                !destroyed && festd::find(deferredTargets, command.m_target.m_id) != deferredTargets.end();
            const bool tokenCommand = command.m_target.m_token.m_list || command.m_parent.m_token.m_list;
            const bool isImmediate =
                command.m_target.m_id.m_value && !tokenCommand && !needsPublication && command.m_kind != CommandKind::kParent;
            if (destroyed && command.m_kind == CommandKind::kComponent)
            {
                command.m_component->m_type->m_destructor(command.m_payload);
                continue;
            }
            if (isImmediate)
                immediate->m_commands.push_back(command);
            else
                deferred.push_back(command);
        }


        list.m_commands = std::move(deferred);
        list.m_eligibleEpoch = m_impl->m_epoch + 1;
        if (!immediate->m_commands.empty())
            m_impl->m_commands.push_back(immediate);
        else
            Memory::DefaultDelete(immediate);

        m_impl->m_commands.push_back(std::exchange(commands.m_impl, nullptr));
    }


    bool EntityWorld::CommitBootstrap()
    {
        if (m_impl->m_started)
            return Fail("Bootstrap is closed after the first update");
        return CommitImpl(true);
    }


    bool EntityWorld::Commit()
    {
        return CommitImpl(false);
    }


    bool EntityWorld::CommitImpl(const bool bootstrap)
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        if (m_impl->m_collecting || m_impl->m_executing)
            return Fail("Structural commit requires a safe boundary");
        m_impl->m_error = {};
        festd::vector<EntityCommandList::Impl*> ready;
        {
            std::lock_guard lock{ m_impl->m_commandLock };
            for (auto it = m_impl->m_commands.begin(); it != m_impl->m_commands.end();)
            {
                if (!bootstrap && (*it)->m_eligibleEpoch > m_impl->m_epoch)
                {
                    ++it;
                    continue;
                }
                ready.push_back(*it);
                it = m_impl->m_commands.erase(it);
            }
        }


        auto cleanup = festd::defer([&] {
            for (auto* list : ready)
                Memory::DefaultDelete(list);
        });
        auto destructionContains = [&](const Command& command, EntityID target) {
            if (command.m_kind != CommandKind::kDestroy)
                return false;
            for (const Entity* current = Find(target); current; current = current->GetParent())
            {
                if (current->GetID() == command.m_target.m_id)
                    return true;
            }
            return false;
        };


        festd::vector<bool> conflicts(ready.size(), false);
        for (uint32_t i = 0; i < ready.size(); ++i)
        {
            for (uint32_t j = i + 1; j < ready.size(); ++j)
            {
                if (ready[i]->m_id == ready[j]->m_id)
                    continue;
                for (const auto& a : ready[i]->m_commands)
                {
                    for (const auto& b : ready[j]->m_commands)
                    {
                        bool same = a.m_target.m_id.m_value != 0 && a.m_target.m_id == b.m_target.m_id;
                        same |= destructionContains(a, b.m_target.m_id) || destructionContains(b, a.m_target.m_id);
                        same |= destructionContains(a, b.m_parent.m_id) || destructionContains(b, a.m_parent.m_id);
                        if (a.m_kind == CommandKind::kParent && b.m_kind == CommandKind::kParent)
                        {
                            same |= a.m_parent.m_id.m_value && a.m_parent.m_id == b.m_target.m_id;
                            same |= b.m_parent.m_id.m_value && b.m_parent.m_id == a.m_target.m_id;
                        }
                        if (a.m_kind == CommandKind::kUnloadRegistry || b.m_kind == CommandKind::kUnloadRegistry)
                        {
                            Entity* ae = Find(a.m_target.m_id);
                            Entity* be = Find(b.m_target.m_id);
                            const auto* ar = a.m_registry ? a.m_registry : ae ? ae->m_registry : nullptr;
                            const auto* br = b.m_registry ? b.m_registry : be ? be->m_registry : nullptr;
                            same |= ar && ar == br;
                        }
                        if (same)
                            conflicts[i] = conflicts[j] = true;
                    }
                }
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
            };
            festd::vector<Edit> edits;
            festd::vector<uint32_t> tokens(list.m_created, kInvalidIndex);
            festd::vector<EntityRegistry*> unloadRegistries;
            for (const auto& slot : m_impl->m_slots)
            {
                if (!slot.m_entity)
                    continue;
                const Entity& entity = *slot.m_entity;
                Edit edit;
                edit.m_entity = slot.m_entity;
                edit.m_registry = entity.m_registry;
                edit.m_uuid = entity.m_uuid;
                edit.m_name = entity.m_name;
                edit.m_wantsActive = entity.m_wantsActive;
                for (const auto* info : entity.m_chunk->m_archetype.m_columns)
                    edit.m_values.push_back({ info, nullptr });

                edits.push_back(std::move(edit));
            }


            auto entityIndex = [&](Entity* entity) {
                if (!entity)
                    return kInvalidIndex;
                for (uint32_t i = 0; i < edits.size(); ++i)
                {
                    if (edits[i].m_entity == entity)
                        return i;
                }
                return kInvalidIndex;
            };
            for (auto& edit : edits)
                edit.m_parent = entityIndex(edit.m_entity->m_parent);
            auto resolve = [&](const EntityTarget& target) {
                if (target.m_token.m_list)
                {
                    if (target.m_token.m_list != list.m_id || target.m_token.m_index >= tokens.size())
                        return kInvalidIndex;
                    return tokens[target.m_token.m_index];
                }
                return entityIndex(Find(target.m_id));
            };
            struct EditContext
            {
                festd::vector<Edit>* m_edits;
                EntityCommandList::Impl* m_list;
            } editContext{ &edits, &list };
            auto component = [](void* data, const uint32_t index, Rtti::TypeID type, const bool write) -> void* {
                auto& context = *static_cast<EditContext*>(data);
                if (index >= context.m_edits->size())
                    return nullptr;
                auto& edit = (*context.m_edits)[index];
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
            bool valid = true;
            for (const auto& command : list.m_commands)
            {
                if (command.m_kind == CommandKind::kCreate)
                {
                    if (festd::find(m_impl->m_registries, command.m_registry) == m_impl->m_registries.end())
                    {
                        valid = Fail("Creation targets a foreign or unloaded registry");
                        break;
                    }
                    if (command.m_registry->GetID() != command.m_registryId)
                    {
                        valid = Fail("Creation registry identity is stale");
                        break;
                    }
                    for (const auto& edit : edits)
                    {
                        if (edit.m_uuid == command.m_uuid)
                            valid = Fail("Duplicate entity UUID");
                    }
                    Edit edit;
                    edit.m_registry = command.m_registry;
                    edit.m_uuid = command.m_uuid;
                    edit.m_name = command.m_name;
                    edit.m_residency = command.m_residency;
                    edit.m_created = edit.m_touched = true;
                    tokens[command.m_target.m_token.m_index] = edits.size();

                    edits.push_back(std::move(edit));
                    continue;
                }
                if (command.m_kind == CommandKind::kUnloadRegistry)
                {
                    if (festd::find(m_impl->m_registries, command.m_registry) == m_impl->m_registries.end())
                        valid = Fail("Unload targets a foreign or unloaded registry");
                    else if (command.m_registry->GetID() != command.m_registryId)
                        valid = Fail("Unload registry identity is stale");
                    else
                        unloadRegistries.push_back(command.m_registry);
                    continue;
                }
                const uint32_t index = resolve(command.m_target);
                if (index == kInvalidIndex)
                {
                    valid = Fail("Stale entity ID or invalid list-local token");
                    break;
                }
                auto& edit = edits[index];
                if (edit.m_destroy)
                    continue;
                edit.m_touched = true;
                switch (command.m_kind)
                {
                case CommandKind::kDestroy:
                    edit.m_destroy = true;
                    for (uint32_t descendant = 0; descendant < edits.size(); ++descendant)
                    {
                        uint32_t ancestor = edits[descendant].m_parent;
                        uint32_t depth = 0;
                        while (ancestor != kInvalidIndex && depth++ < edits.size())
                        {
                            if (ancestor == index)
                            {
                                edits[descendant].m_destroy = true;
                                edits[descendant].m_touched = true;
                                break;
                            }
                            ancestor = edits[ancestor].m_parent;
                        }
                    }
                    break;
                case CommandKind::kRename:
                    edit.m_name = command.m_name;
                    break;
                case CommandKind::kActive:
                    edit.m_wantsActive = command.m_active;
                    break;
                case CommandKind::kUnload:
                    edit.m_unload = true;
                    edit.m_wantsActive = false;
                    break;
                case CommandKind::kParent:
                    {
                        const bool nullParent = !command.m_parent.m_id.m_value && !command.m_parent.m_token.m_list;
                        const uint32_t parent = nullParent ? kInvalidIndex : resolve(command.m_parent);
                        if (!nullParent && parent == kInvalidIndex)
                            valid = Fail("Invalid parent handle");
                        else if (parent != kInvalidIndex && edits[parent].m_registry != edit.m_registry)
                            valid = Fail("Parenting cannot cross registry or world boundaries");
                        else
                        {
                            if (edit.m_parent != parent && m_impl->m_reparentHandler
                                && command.m_reparentMode == ReparentMode::kPreserveWorld)
                            {
                                ReparentContext context{ index,
                                                         parent,
                                                         static_cast<uint32_t>(edits.size()),
                                                         command.m_reparentMode,
                                                         &editContext,
                                                         [](void* data, uint32_t target) {
                                                             return (*static_cast<EditContext*>(data)->m_edits)[target].m_parent;
                                                         },
                                                         component };
                                if (!m_impl->m_reparentHandler(context))
                                {
                                    valid = Fail("Reparent transform validation failed; original values and hierarchy retained");
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
                        const auto found = festd::find_if(edit.m_values.begin(), edit.m_values.end(), [&](const Value& value) {
                            return value.m_info->m_type->m_id == command.m_type;
                        });
                        if (found != edit.m_values.end())
                            edit.m_values.erase(found);
                        if (command.m_kind == CommandKind::kComponent)
                            edit.m_values.push_back({ command.m_component, command.m_payload });
                        edit.m_componentsChanged = true;
                        break;
                    }
                default:
                    break;
                }
                if (!valid)
                    break;
            }
            for (uint32_t i = 0; valid && i < edits.size(); ++i)
            {
                uint32_t ancestor = edits[i].m_parent;
                if (ancestor != kInvalidIndex && edits[ancestor].m_destroy && !edits[i].m_destroy)
                {
                    valid = Fail("Cannot parent an entity to a target queued for destruction");
                    break;
                }
                uint32_t depth = 0;
                while (ancestor != kInvalidIndex)
                {
                    if (ancestor == i || ++depth > edits.size())
                    {
                        valid = Fail("Hierarchy cycle rejected before changing links");
                        break;
                    }
                    ancestor = edits[ancestor].m_parent;
                }
                auto& edit = edits[i];
                if (!edit.m_touched || edit.m_destroy)
                    continue;

                if (festd::find(unloadRegistries, edit.m_registry) != unloadRegistries.end())
                {
                    valid = Fail("Cannot edit and unload the same registry in one batch");
                    break;
                }

                festd::sort(edit.m_values.begin(), edit.m_values.end(), [](const Value& a, const Value& b) {
                    return a.m_info->m_type->m_id < b.m_info->m_type->m_id;
                });
                festd::vector<const EntityComponentInfo*> columns;
                for (const auto& value : edit.m_values)
                    columns.push_back(value.m_info);
                Archetype validation(columns);
                if (validation.m_lifecycleOrder.size() != columns.size())
                    valid = Fail("Missing or cyclic component initialization dependency");
            }
            if (!valid)
            {
                success = false;
                continue;
            }
            // No entity/link/storage mutation precedes validation of the complete list.
            for (auto& edit : edits)
            {
                if (edit.m_created && !edit.m_destroy)
                {
                    edit.m_entity = AllocateEntity(*edit.m_registry, edit.m_name, edit.m_uuid);
                    edit.m_entity->m_runtime->m_residencyScope = edit.m_residency;
                }
            }
            for (auto& edit : edits)
            {
                if (!edit.m_touched || !edit.m_entity)
                    continue;
                Entity& entity = *edit.m_entity;
                entity.m_name = edit.m_name;
                if (edit.m_created || edit.m_componentsChanged)
                {
                    festd::vector<const EntityComponentInfo*> columns;
                    festd::vector<void*> values;
                    for (const auto& value : edit.m_values)
                    {
                        columns.push_back(value.m_info);
                        values.push_back(value.m_value);
                    }
                    Migrate(entity, columns, values);
                    entity.m_failed = false;
                }
                if (entity.m_wantsActive != edit.m_wantsActive)
                    MarkUnready(entity);
                entity.m_wantsActive = edit.m_wantsActive;
            }
            // Detach changed links first so a valid final topology cannot temporarily form a cycle.
            for (auto& edit : edits)
            {
                if (edit.m_parentChanged && edit.m_entity)
                    Reparent(*edit.m_entity, nullptr);
            }
            for (auto& edit : edits)
            {
                if (edit.m_parentChanged && edit.m_entity && edit.m_parent != kInvalidIndex)
                    Reparent(*edit.m_entity, edits[edit.m_parent].m_entity);
            }
            for (auto& edit : edits)
            {
                Entity* entity = Find(edit.m_uuid, false);
                if (!entity || !edit.m_touched)
                    continue;
                if (!edit.m_wantsActive)
                    DeactivateSubtree(*entity);
                if (edit.m_unload)
                {
                    // Unload keeps allocated identity but resets runtime state, descendants first.
                    auto unload = [&](auto&& self, Entity& current) -> void {
                        for (Entity* child = current.m_firstChild; child; child = child->m_nextSibling)
                            self(self, *child);
                        const auto& order = current.m_chunk->m_archetype.m_lifecycleOrder;
                        for (uint32_t j = order.size(); j > 0; --j)
                            TeardownComponent(current, order[j - 1], false);
                        current.m_runtime->m_prepared = false;
                        current.m_wantsActive = false;
                    };
                    unload(unload, *entity);
                }
                if (edit.m_destroy)
                    DestroyEntity(*entity);
            }
            for (auto* registry : unloadRegistries)
                RemoveRegistry(*registry);
        }
        AdvanceMaterializations(bootstrap);
        return success;
    }
} // namespace FE::Framework
