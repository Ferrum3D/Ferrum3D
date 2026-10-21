#include <Core/Serialization/DependencySerialization.h>
#include <Framework/Entities/EntityWorldInternal.h>

namespace FE::Framework
{
    namespace
    {
        bool HasStage(const ComponentStage stage, const ComponentStage flags)
        {
            return (stage & flags) != ComponentStage::kNone;
        }


        // Compute the following node before invoking cleanup, so callbacks may destroy the visited entity.
        template<class Callable>
        void VisitSubtreePostOrder(Entity& root, Callable&& callable)
        {
            Entity* entity = &root;
            while (entity->GetFirstChild())
                entity = entity->GetFirstChild();

            while (entity)
            {
                Entity* next = nullptr;
                if (entity != &root)
                {
                    next = entity->GetNextSibling();
                    if (next)
                    {
                        while (next->GetFirstChild())
                            next = next->GetFirstChild();
                    }
                    else
                    {
                        next = entity->GetParent();
                    }
                }

                callable(*entity);
                entity = next;
            }
        }
    } // namespace


    EntityLifecycle::EntityLifecycle(EntityWorld::Impl& world)
        : m_world(world)
    {
    }


    bool EntityLifecycle::HasPendingReplacements(const Entity& entity)
    {
        return entity.m_resources && !entity.m_resources->m_replacements.empty();
    }


    EntityResources& EntityLifecycle::GetResources(Entity& entity)
    {
        if (!entity.m_resources)
            entity.m_resources = Memory::DefaultNew<EntityResources>(*m_world.m_assets);

        return *entity.m_resources;
    }


    EntityResidencySet& EntityLifecycle::ResidencyOwner(Entity& entity)
    {
        if (entity.m_residencyScope == ResidencyScope::kRegistry)
            return entity.m_registry->m_residency;

        return GetResources(entity).m_residency;
    }


    void EntityLifecycle::ContributeAsset(Entity& entity, const LoadingScope& scope, const IO::AssetID id,
                                          const Rtti::TypeID type)
    {
        if (!id.IsValid())
            return;

        auto& contributions = GetResources(entity).m_assets;
        for (const auto& contribution : contributions)
        {
            const bool sameComponent =
                contribution.m_component == scope.m_component && contribution.m_transition == scope.m_transition;
            const bool sameAsset = contribution.m_asset == id && contribution.m_expectedType == type;
            if (sameComponent && sameAsset)
                return;
        }

        ResidencyOwner(entity).Add(id, type);
        contributions.push_back({ scope.m_component, id, type, scope.m_transition });
    }


    void EntityLifecycle::RequireAsset(Entity& entity, const IO::AssetID id, const Rtti::TypeID type)
    {
        FE_Assert(m_loading.m_component.IsValid(), "Require is legal only during dependency discovery or Load");
        ContributeAsset(entity, m_loading, id, type);
    }


    void EntityLifecycle::InstallDependencies(Entity& entity, const uint32_t column,
                                              const festd::span<const AssetDependency> dependencies)
    {
        const LoadingScope scope{ entity.m_chunk->m_archetype.m_columns[column]->m_type->m_id };
        for (const auto& [asset, type] : dependencies)
            ContributeAsset(entity, scope, asset, type);

        entity.m_chunk->Stage(entity.m_row, column) |= ComponentStage::kDiscovered;
    }


    void EntityLifecycle::ReleaseAssets(Entity& entity, const Rtti::TypeID component, const uint64_t transition)
    {
        if (!entity.m_resources)
            return;

        auto& assets = entity.m_resources->m_assets;
        for (auto it = assets.begin(); it != assets.end();)
        {
            if (it->m_component != component || it->m_transition != transition)
            {
                ++it;
                continue;
            }

            ResidencyOwner(entity).Remove(it->m_asset, it->m_expectedType);
            it = assets.erase(it);
        }
    }


    LifecycleResult EntityLifecycle::PollDependencies(Entity& entity, const LoadingScope& scope)
    {
        if (!entity.m_resources)
            return LifecycleResult::kSucceeded;

        LifecycleResult result = LifecycleResult::kSucceeded;
        for (const auto& contribution : entity.m_resources->m_assets)
        {
            if (contribution.m_component != scope.m_component || contribution.m_transition != scope.m_transition)
                continue;

            const auto status = ResidencyOwner(entity).Poll(contribution.m_asset, contribution.m_expectedType);
            if (status == LifecycleResult::kFailed)
                return status;

            if (status == LifecycleResult::kPending)
                result = status;
        }

        return result;
    }


    LifecycleResult EntityLifecycle::LoadComponent(Entity& entity, const uint32_t column)
    {
        auto& chunk = *entity.m_chunk;
        return LoadValue(entity,
                         *chunk.m_archetype.m_columns[column],
                         chunk.Get(entity.m_row, column),
                         chunk.Stage(entity.m_row, column));
    }


    LifecycleResult EntityLifecycle::LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                                               const uint64_t transition)
    {
        if (HasStage(stage, ComponentStage::kLoaded))
            return LifecycleResult::kSucceeded;

        m_loading = { info.m_type->m_id, transition };
        auto loadingScope = festd::defer([&] {
            m_loading = {};
        });

        if (!HasStage(stage, ComponentStage::kDiscovered))
        {
            stage |= ComponentStage::kDiscovered;
            if (info.m_type->m_serialize)
            {
                struct Discovery
                {
                    EntityLifecycle* m_lifecycle;
                    Entity* m_entity;
                } discovery{ this, &entity };

                const auto result = Serialization::VisitAssetReferences(
                    *info.m_type,
                    data,
                    &discovery,
                    [](void* user, const Uuid asset, const Rtti::TypeID type, const uint32_t kind) {
                        if (kind != festd::to_underlying(IO::DependencyKind::kHard))
                            return;

                        auto& d = *static_cast<Discovery*>(user);
                        d.m_lifecycle->ContributeAsset(*d.m_entity, d.m_lifecycle->m_loading, asset, type);
                    });

                if (result != Serialization::ResultCode::kSuccess)
                    return LifecycleResult::kFailed;
            }
        }

        auto status = PollDependencies(entity, m_loading);
        if (status == LifecycleResult::kSucceeded)
        {
            ComponentLoadingContext loading{ { entity, m_world.m_owner } };
            stage |= ComponentStage::kLoading;
            status = info.m_load ? info.m_load(data, loading) : LifecycleResult::kSucceeded;
            if (status == LifecycleResult::kSucceeded)
                status = PollDependencies(entity, m_loading);
        }

        if (status == LifecycleResult::kSucceeded)
            stage |= ComponentStage::kLoaded;

        return status;
    }


    void EntityLifecycle::TeardownComponent(Entity& entity, const uint32_t column, const bool destroy)
    {
        auto& chunk = *entity.m_chunk;
        const auto& info = *chunk.m_archetype.m_columns[column];
        void* data = chunk.Get(entity.m_row, column);

        TeardownValue(entity, info, data, chunk.Stage(entity.m_row, column));
        if (destroy)
            info.m_type->m_destructor(data);
    }


    void EntityLifecycle::TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                                        const uint64_t transition)
    {
        ComponentContext context{ entity, m_world.m_owner };
        ComponentLoadingContext loading{ context };

        if (HasStage(stage, ComponentStage::kActive) && info.m_deactivate)
            info.m_deactivate(data, context);
        if (HasStage(stage, ComponentStage::kInitialized) && info.m_shutdown)
            info.m_shutdown(data, context);
        if (HasStage(stage, ComponentStage::kLoading) && info.m_unload)
            info.m_unload(data, loading);

        ReleaseAssets(entity, info.m_type->m_id, transition);
        stage = ComponentStage::kNone;
    }


    void EntityLifecycle::StageReplacement(Entity& entity, const EntityComponentInfo& info, void* value)
    {
        const auto& type = *info.m_type;
        CancelReplacements(entity, type.m_id);

        void* data = Memory::DefaultAllocate(type.m_size, type.m_alignment);
        type.m_moveConstructor(data, value);
        GetResources(entity).m_replacements.push_back({ &info, data, m_nextTransition++ });

        if (festd::find(m_replacements, entity.m_id) == m_replacements.end())
            m_replacements.push_back(entity.m_id);
    }


    void EntityLifecycle::CancelReplacements(Entity& entity, const Rtti::TypeID type, const bool keepAuthoredValues)
    {
        if (!entity.m_resources)
            return;

        auto& replacements = entity.m_resources->m_replacements;

        bool valuesChanged = false;
        for (auto it = replacements.begin(); it != replacements.end();)
        {
            if (type.IsValid() && it->m_info->m_type->m_id != type)
            {
                ++it;
                continue;
            }

            TeardownValue(entity, *it->m_info, it->m_data, it->m_stage, it->m_transition);
            if (keepAuthoredValues)
            {
                FE_AssertDebug(!entity.m_active);

                const uint32_t column = entity.m_chunk->m_archetype.Find(it->m_info->m_type->m_id);
                FE_AssertDebug(column != kInvalidIndex);

                TeardownComponent(entity, column, true);
                it->m_info->m_type->m_moveConstructor(entity.m_chunk->Get(entity.m_row, column), it->m_data);
                entity.m_chunk->Stage(entity.m_row, column) = ComponentStage::kNone;
                valuesChanged = true;
            }

            it->m_info->m_type->m_destructor(it->m_data);
            Memory::DefaultFree(it->m_data);
            it = replacements.erase(it);
        }

        if (valuesChanged)
        {
            MarkUnready(entity);
            m_world.MarkChanged(entity);
        }
    }


    void EntityLifecycle::AdvanceReplacements(Entity& entity)
    {
        if (!entity.m_active || !entity.m_wantsActive || !entity.m_resources)
            return;

        auto& replacements = entity.m_resources->m_replacements;
        for (auto it = replacements.begin(); it != replacements.end();)
        {
            auto& replacement = *it;
            const auto& info = *replacement.m_info;
            auto result = LoadValue(entity, info, replacement.m_data, replacement.m_stage, replacement.m_transition);
            if (result == LifecycleResult::kPending)
            {
                ++it;
                continue;
            }

            ComponentContext context{ entity, m_world.m_owner };
            if (result == LifecycleResult::kSucceeded)
            {
                replacement.m_stage |= ComponentStage::kInitialized;
                if (info.m_init)
                    result = info.m_init(replacement.m_data, context);
            }

            if (result == LifecycleResult::kSucceeded)
            {
                replacement.m_stage |= ComponentStage::kActive;
                m_world.MarkChanged(entity);
                if (info.m_activate)
                    result = info.m_activate(replacement.m_data, context);
            }

            if (result == LifecycleResult::kSucceeded)
            {
                const uint32_t column = entity.m_chunk->m_archetype.Find(info.m_type->m_id);
                TeardownComponent(entity, column, true);

                info.m_type->m_moveConstructor(entity.m_chunk->Get(entity.m_row, column), replacement.m_data);
                entity.m_chunk->Stage(entity.m_row, column) = replacement.m_stage;

                for (auto& contribution : entity.m_resources->m_assets)
                {
                    if (contribution.m_transition == replacement.m_transition)
                        contribution.m_transition = 0;
                }
            }
            else
            {
                TeardownValue(entity, info, replacement.m_data, replacement.m_stage, replacement.m_transition);
                m_world.Fail("Component replacement failed; previous active value retained");
            }

            info.m_type->m_destructor(replacement.m_data);
            Memory::DefaultFree(replacement.m_data);
            it = replacements.erase(it);
        }
    }


    void EntityLifecycle::DeactivateSubtree(Entity& entity, const bool keepAuthoredValues)
    {
        VisitSubtreePostOrder(entity, [&](Entity& current) {
            DeactivateEntity(current, keepAuthoredValues);
        });
    }


    void EntityLifecycle::DeactivateEntity(Entity& entity, const bool keepAuthoredValues)
    {
        ComponentContext context{ entity, m_world.m_owner };
        for (uint32_t i = entity.m_chunk->m_archetype.m_columns.size(); i > 0; --i)
        {
            const uint32_t column = i - 1;
            auto& stage = entity.m_chunk->Stage(entity.m_row, column);
            if (!HasStage(stage, ComponentStage::kActive))
                continue;

            const auto& info = *entity.m_chunk->m_archetype.m_columns[column];
            if (info.m_deactivate)
                info.m_deactivate(entity.m_chunk->Get(entity.m_row, column), context);
            stage &= ~ComponentStage::kActive;
        }

        entity.m_active = false;
        entity.m_activationPrepared = false;
        CancelReplacements(entity, Rtti::TypeID::kNull, keepAuthoredValues);
    }


    void EntityLifecycle::UnloadSubtree(Entity& entity)
    {
        VisitSubtreePostOrder(entity, [&](Entity& current) {
            for (uint32_t i = current.m_chunk->m_archetype.m_columns.size(); i > 0; --i)
                TeardownComponent(current, i - 1, false);

            current.m_prepared = false;
            current.m_wantsActive = false;
        });
    }


    void EntityLifecycle::UnwindSubtree(Entity& entity)
    {
        // Complete one children-first deactivation pass before tearing down any component.
        DeactivateSubtree(entity, false);
        VisitSubtreePostOrder(entity, [&](Entity& current) {
            for (uint32_t i = current.m_chunk->m_archetype.m_columns.size(); i > 0; --i)
                TeardownComponent(current, i - 1, false);

            current.m_failed = true;
            current.m_prepared = false;
        });
    }


    void EntityLifecycle::MarkUnready(Entity& entity)
    {
        entity.m_activationPrepared = false;
        Entity* root = &entity;
        for (Entity* current = &entity; current; current = current->m_parent)
        {
            current->m_prepared = false;
            root = current;
        }

        if (festd::find(m_roots, root->m_id) == m_roots.end())
            m_roots.push_back(root->m_id);
    }


    bool EntityLifecycle::AdmitEntity(Entity& entity)
    {
        if (!m_world.m_started)
            return true;

        const uint64_t frame = m_world.GetQuotaFrame();
        uint32_t& remaining = m_quota.Refresh(frame, kEntitiesPerFrame);
        if (entity.m_lifecycleEpoch == frame)
            return true;

        if (remaining == 0)
            return false;

        --remaining;
        entity.m_lifecycleEpoch = frame;
        return true;
    }


    bool EntityLifecycle::PrepareSubtree(Entity& entity)
    {
        if (entity.m_failed || !entity.m_wantsActive)
            return false;

        auto& chunk = *entity.m_chunk;
        const uint32_t columnCount = chunk.m_archetype.m_columns.size();
        auto failComponent = [&](const uint32_t column) {
            if (entity.m_active)
            {
                TeardownComponent(entity, column, false);
                chunk.Stage(entity.m_row, column) = ComponentStage::kFailed;
            }
            else
            {
                entity.m_failed = true;
            }
        };

        bool ownLoaded = true;
        for (uint32_t column = 0; column < columnCount; ++column)
        {
            if (HasStage(chunk.Stage(entity.m_row, column), ComponentStage::kLoaded | ComponentStage::kFailed))
                continue;

            const auto status = LoadComponent(entity, column);
            if (status == LifecycleResult::kFailed)
            {
                failComponent(column);
                return false;
            }

            ownLoaded &= status == LifecycleResult::kSucceeded;
        }

        // Child readiness is retained between polls; only pending subtrees are revisited.
        bool areChildrenReady = true;
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            const bool childReady = child->m_prepared || PrepareSubtree(*child);
            if (!childReady && (!entity.m_active || !child->m_failed))
                areChildrenReady = false;

            if (child->m_failed)
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);
            }
        }

        const bool subtreeLoading = !ownLoaded || !areChildrenReady;
        if (entity.m_failed || (!entity.m_active && subtreeLoading))
            return false;

        const auto isSettled = [&](const uint32_t column) {
            return HasStage(chunk.Stage(entity.m_row, column), ComponentStage::kInitialized | ComponentStage::kFailed);
        };

        bool needsInit = false;
        for (uint32_t column = 0; column < columnCount; ++column)
            needsInit |= !isSettled(column);

        if (needsInit && !AdmitEntity(entity))
            return false;

        ComponentContext context{ entity, m_world.m_owner };
        for (uint32_t column = 0; column < columnCount; ++column)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            if (isSettled(column) || !HasStage(stage, ComponentStage::kLoaded))
                continue;

            const auto& info = *chunk.m_archetype.m_columns[column];
            // Undo is required even when a synchronous transition reports failure after partial work.
            stage |= ComponentStage::kInitialized;
            if (info.m_init && info.m_init(chunk.Get(entity.m_row, column), context) != LifecycleResult::kSucceeded)
            {
                failComponent(column);
                return false;
            }
        }

        bool ownPrepared = true;
        for (uint32_t column = 0; column < columnCount; ++column)
            ownPrepared &= isSettled(column);

        entity.m_prepared = ownPrepared && areChildrenReady;
        return entity.m_prepared;
    }


    LifecycleResult EntityLifecycle::ActivateSubtree(Entity& entity)
    {
        if (!entity.m_wantsActive || entity.m_failed)
            return LifecycleResult::kFailed;

        if (!entity.m_activationPrepared && !AdmitEntity(entity))
            return LifecycleResult::kPending;

        auto& chunk = *entity.m_chunk;
        ComponentContext context{ entity, m_world.m_owner };
        for (uint32_t column = 0; column < chunk.m_archetype.m_columns.size(); ++column)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            if (!HasStage(stage, ComponentStage::kInitialized) || HasStage(stage, ComponentStage::kActive))
                continue;

            const auto& info = *chunk.m_archetype.m_columns[column];
            stage |= ComponentStage::kActive;
            m_world.MarkChanged(entity);
            if (info.m_activate && info.m_activate(chunk.Get(entity.m_row, column), context) != LifecycleResult::kSucceeded)
            {
                if (entity.m_active)
                {
                    TeardownComponent(entity, column, false);
                    chunk.Stage(entity.m_row, column) = ComponentStage::kFailed;
                }
                else
                {
                    entity.m_failed = true;
                }

                return LifecycleResult::kFailed;
            }

            if (entity.m_active)
                PublishAdditions(entity);
        }

        entity.m_activationPrepared = true;
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            if (!child->m_active && !child->m_prepared)
                continue;

            const LifecycleResult result = ActivateSubtree(*child);
            if (result == LifecycleResult::kPending)
                return LifecycleResult::kPending;

            if (result == LifecycleResult::kFailed)
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);

                return LifecycleResult::kFailed;
            }
        }

        return LifecycleResult::kSucceeded;
    }


    void EntityLifecycle::PublishAdditions(Entity& entity)
    {
        auto& storage = m_world.m_storage;
        uint64_t version = 0;
        auto nextVersion = [&] {
            if (version == 0)
            {
                FE_Assert(storage.m_additionVersion != UINT64_MAX, "Addition publication serial exhausted");
                version = ++storage.m_additionVersion;
            }

            return version;
        };

        if (entity.m_addedVersion == 0)
            entity.m_addedVersion = nextVersion();

        auto& chunk = *entity.m_chunk;
        for (uint32_t column = 0; column < chunk.m_archetype.m_columns.size(); ++column)
        {
            const bool active = HasStage(chunk.Stage(entity.m_row, column), ComponentStage::kActive);
            if (active && chunk.AddedVersion(entity.m_row, column) == 0)
                chunk.AddedVersion(entity.m_row, column) = nextVersion();
        }
    }


    void EntityLifecycle::PublishSubtree(Entity& entity)
    {
        entity.m_active = true;
        PublishAdditions(entity);
        m_world.MarkChanged(entity);

        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (child->m_prepared && child->m_wantsActive && !child->m_failed)
                PublishSubtree(*child);
        }
    }


    void EntityLifecycle::Advance()
    {
        FE_PROFILER_ZONE();

        // Replacements finish atomically before group readiness and publication are reconsidered.
        for (const EntityID id : m_replacements)
        {
            if (Entity* entity = m_world.m_owner.Find(id))
                AdvanceReplacements(*entity);
        }

        m_replacements.erase(std::remove_if(m_replacements.begin(),
                                            m_replacements.end(),
                                            [&](const EntityID id) {
                                                const Entity* entity = m_world.m_owner.Find(id);
                                                return !entity || !HasPendingReplacements(*entity);
                                            }),
                             m_replacements.end());

        const uint32_t rootCount = m_roots.size();
        for (uint32_t i = 0; i < rootCount; ++i)
        {
            Entity* entity = m_world.m_owner.Find(m_roots[i]);
            if (!entity || entity->m_parent || !entity->m_wantsActive || entity->m_failed)
                continue;

            const bool wasActive = entity->m_active;
            if (wasActive && entity->m_prepared)
                continue;

            PrepareSubtree(*entity);
            if (entity->m_failed)
            {
                if (!wasActive)
                    UnwindSubtree(*entity);

                continue;
            }

            if (!entity->m_prepared && !wasActive)
                continue;

            const LifecycleResult result = ActivateSubtree(*entity);
            if (result == LifecycleResult::kSucceeded)
                PublishSubtree(*entity);
            else if (result == LifecycleResult::kFailed && !wasActive)
                UnwindSubtree(*entity);
        }

        m_roots.erase(std::remove_if(m_roots.begin(),
                                     m_roots.end(),
                                     [&](const EntityID id) {
                                         const Entity* entity = m_world.m_owner.Find(id);
                                         return !entity || entity->m_parent || !entity->m_wantsActive || entity->m_failed
                                             || (entity->m_active && entity->m_prepared);
                                     }),
                      m_roots.end());
    }
} // namespace FE::Framework
