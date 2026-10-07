#include <Core/IO/MemoryStream.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    namespace
    {
        // Serialize authored fields only to visit references; no redundant payload buffers are produced.
        struct DependencyOnlyFormat final : Serialization::SerializationFormat
        {
            DependencyOnlyFormat()
                : SerializationFormat(Serialization::Format::kPackedBinary)
            {
            }


            void ResetImpl() override {}


            Serialization::ResultCode BeginStoreDocumentImpl(Rtti::TypeID, uint32_t, uint64_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadDocumentImpl(Rtti::TypeID, uint32_t, uint64_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreDocumentImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadDocumentImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreFieldImpl(festd::ascii_view, uint64_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadFieldImpl(festd::ascii_view, uint64_t, bool&) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreFieldImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadFieldImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreArrayImpl(uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadArrayImpl(uint32_t&) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreArrayImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadArrayImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreElementImpl(uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadElementImpl(uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreElementImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadElementImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode StoreScalarImpl(Serialization::ScalarKind, const void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadScalarImpl(Serialization::ScalarKind, void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode StoreBytesImpl(const void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadBytesImpl(void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode StoreStringImpl(festd::string_view) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadStringSizeImpl(uint32_t&) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadStringImpl(festd::span<char>) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            uint64_t GetStoreCurrentOffsetImpl() const override
            {
                return 0;
            }


            uint64_t GetLoadCurrentOffsetImpl() const override
            {
                return 0;
            }
        };


    } // namespace

    EntityResidencySet& EntityWorld::ResidencyOwner(Entity& entity)
    {
        if (entity.m_runtime->m_residencyScope == ResidencyScope::kRegistry)
            return entity.m_registry->m_residency;
        if (!entity.m_runtime->m_residency)
            entity.m_runtime->m_residency = Memory::DefaultNew<EntityResidencySet>(*m_impl->m_assets);
        return *entity.m_runtime->m_residency;
    }


    void EntityWorld::ReleaseAssets(Entity& entity, const Rtti::TypeID component, const uint64_t transition)
    {
        auto& assets = entity.m_runtime->m_assets;
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


    void EntityWorld::TeardownComponent(Entity& entity, const uint32_t column, const bool destroy)
    {
        auto& chunk = *entity.m_chunk;
        const auto& info = *chunk.m_archetype.m_columns[column];
        void* data = chunk.Get(entity.m_row, column);
        auto& stage = chunk.Stage(entity.m_row, column);
        TeardownValue(entity, info, data, stage);
        if (destroy)
            info.m_type->m_destructor(data);
    }


    void EntityWorld::TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, uint8_t& stage,
                                    const uint64_t transition)
    {
        ComponentContext context{ entity, *this, m_impl->m_services };
        ComponentLoadingContext loading{ context };
        if ((stage & kActive) && info.m_deactivate)
            info.m_deactivate(data, context);
        if ((stage & kInitialized) && info.m_shutdown)
            info.m_shutdown(data, context);
        if ((stage & kLoading) && info.m_unload)
            info.m_unload(data, loading);
        ReleaseAssets(entity, info.m_type->m_id, transition);
        stage = 0;
    }


    void EntityWorld::CancelReplacements(Entity& entity, const Rtti::TypeID type, const bool keepAuthoredValues)
    {
        auto& replacements = entity.m_runtime->m_replacements;
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
                FE_Assert(!entity.m_active);
                const uint32_t column = entity.m_chunk->m_archetype.Find(it->m_info->m_type->m_id);
                FE_Assert(column != kInvalidIndex);
                TeardownComponent(entity, column, true);
                it->m_info->m_type->m_moveConstructor(entity.m_chunk->Get(entity.m_row, column), it->m_data);
                entity.m_chunk->Stage(entity.m_row, column) = 0;
                valuesChanged = true;
            }

            it->m_info->m_type->m_destructor(it->m_data);
            Memory::DefaultFree(it->m_data);
            it = replacements.erase(it);
        }

        if (valuesChanged)
        {
            MarkUnready(entity);
            MarkChanged(entity);
        }
    }


    void EntityWorld::DeactivateSubtree(Entity& entity, const bool keepAuthoredValues)
    {
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
            DeactivateSubtree(*child, keepAuthoredValues);

        const auto& order = entity.m_chunk->m_archetype.m_lifecycleOrder;
        ComponentContext context{ entity, *this, m_impl->m_services };
        for (uint32_t i = order.size(); i > 0; --i)
        {
            const uint32_t column = order[i - 1];
            auto& stage = entity.m_chunk->Stage(entity.m_row, column);
            if (!(stage & kActive))
                continue;

            const auto& info = *entity.m_chunk->m_archetype.m_columns[column];
            if (info.m_deactivate)
                info.m_deactivate(entity.m_chunk->Get(entity.m_row, column), context);
            stage &= ~kActive;
        }

        entity.m_active = false;
        CancelReplacements(entity, Rtti::TypeID::kNull, keepAuthoredValues);
    }


    bool EntityWorld::RequireAsset(Entity& entity, const IO::AssetID id, const Rtti::TypeID type)
    {
        FE_Assert(m_impl->m_loadingComponent.IsValid(), "Require is legal only during dependency discovery or Load");
        if (!id.IsValid())
            return true;

        auto& contributions = entity.m_runtime->m_assets;
        for (const auto& contribution : contributions)
        {
            const bool sameComponent = contribution.m_component == m_impl->m_loadingComponent
                && contribution.m_transition == m_impl->m_loadingTransition;
            const bool sameAsset = contribution.m_asset == id && contribution.m_expectedType == type;
            if (sameComponent && sameAsset)
                return true;
        }
        ResidencyOwner(entity).Add(id, type);
        contributions.push_back({ m_impl->m_loadingComponent, id, type, m_impl->m_loadingTransition });
        return true;
    }


    LifecycleResult EntityWorld::LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, uint8_t& stage,
                                           const uint64_t transition)
    {
        ComponentLoadingContext loading{ { entity, *this, m_impl->m_services } };
        if (stage & kLoaded)
            return LifecycleResult::kSucceeded;

        m_impl->m_loadingComponent = info.m_type->m_id;
        m_impl->m_loadingTransition = transition;
        auto loadingScope = festd::defer([&] {
            m_impl->m_loadingComponent = Rtti::TypeID::kNull;
            m_impl->m_loadingTransition = 0;
        });
        if (!(stage & kDiscovered))
        {
            stage |= kDiscovered;
            if (info.m_type->m_serialize)
            {
                struct Discovery
                {
                    EntityWorld* m_world;
                    Entity* m_entity;
                } discovery{ this, &entity };
                IO::WriteOnlyMemoryStream stream;
                DependencyOnlyFormat format;
                Serialization::SerializationContext serialization(&stream,
                                                                  format,
                                                                  &discovery,
                                                                  [](void* user, Uuid asset, Rtti::TypeID type, uint32_t kind) {
                                                                      if (kind != festd::to_underlying(IO::DependencyKind::kHard))
                                                                          return;

                                                                      auto& d = *static_cast<Discovery*>(user);
                                                                      d.m_world->RequireAsset(*d.m_entity, asset, type);
                                                                  });
                if (serialization.Store(*info.m_type, data) != Serialization::ResultCode::kSuccess)
                    return LifecycleResult::kFailed;
            }
        }

        auto dependenciesReady = [&] {
            LifecycleResult result = LifecycleResult::kSucceeded;
            for (const auto& contribution : entity.m_runtime->m_assets)
            {
                if (contribution.m_component != info.m_type->m_id || contribution.m_transition != transition)
                    continue;

                const auto status = ResidencyOwner(entity).Poll(contribution.m_asset, contribution.m_expectedType);
                if (status == LifecycleResult::kFailed)
                    return status;
                if (status == LifecycleResult::kPending)
                    result = status;
            }
            return result;
        };

        auto status = dependenciesReady();
        if (status == LifecycleResult::kSucceeded)
        {
            stage |= kLoading;
            status = info.m_load ? info.m_load(data, loading) : LifecycleResult::kSucceeded;
            if (status == LifecycleResult::kSucceeded)
                status = dependenciesReady();
        }
        if (status == LifecycleResult::kSucceeded)
            stage |= kLoaded;
        return status;
    }


    bool EntityWorld::PrepareSubtree(Entity& entity)
    {
        if (entity.m_failed || !entity.m_wantsActive)
            return false;

        auto& chunk = *entity.m_chunk;
        bool ownLoaded = true;
        auto failComponent = [&](uint32_t column) {
            if (entity.m_active)
            {
                TeardownComponent(entity, column, false);
                chunk.Stage(entity.m_row, column) = kFailed;
            }
            else
                entity.m_failed = true;
        };
        ComponentContext context{ entity, *this, m_impl->m_services };
        for (const uint32_t column : chunk.m_archetype.m_lifecycleOrder)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            const auto& info = *chunk.m_archetype.m_columns[column];
            if (stage & (kLoaded | kFailed))
                continue;

            const auto status = LoadValue(entity, info, chunk.Get(entity.m_row, column), stage);
            if (status == LifecycleResult::kFailed)
            {
                failComponent(column);
                return false;
            }
            if (status == LifecycleResult::kPending)
                ownLoaded = false;
            else
                stage |= kLoaded;
        }

        // Child readiness is retained between polls; only pending subtrees are revisited.
        entity.m_runtime->m_unreadyChildren = 0;
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            const bool childReady = child->m_runtime->m_prepared || PrepareSubtree(*child);
            if (!childReady && (!entity.m_active || !child->m_failed))
                ++entity.m_runtime->m_unreadyChildren;
            if (child->m_failed)
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);
            }
        }

        const bool subtreeLoading = !ownLoaded || entity.m_runtime->m_unreadyChildren != 0;
        if (entity.m_failed || (!entity.m_active && subtreeLoading))
            return false;

        for (const uint32_t column : chunk.m_archetype.m_lifecycleOrder)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            if (stage & (kInitialized | kFailed))
                continue;
            if (!(stage & kLoaded))
                continue;

            const auto& info = *chunk.m_archetype.m_columns[column];
            bool dependenciesInitialized = true;
            for (const auto dependency : info.m_initAfter)
            {
                const uint32_t dependencyColumn = chunk.m_archetype.Find(dependency);
                const uint8_t dependencyStage = chunk.Stage(entity.m_row, dependencyColumn);
                if (dependencyStage & kFailed)
                {
                    failComponent(column);
                    return false;
                }
                dependenciesInitialized &= (dependencyStage & kInitialized) != 0;
            }
            if (!dependenciesInitialized)
                continue;

            // Undo is required even when a synchronous transition reports failure after partial work.
            stage |= kInitialized;
            if (info.m_init && info.m_init(chunk.Get(entity.m_row, column), context) != LifecycleResult::kSucceeded)
            {
                failComponent(column);
                return false;
            }
        }

        bool ownPrepared = true;
        for (uint32_t column = 0; column < chunk.m_archetype.m_columns.size(); ++column)
            ownPrepared &= (chunk.Stage(entity.m_row, column) & (kInitialized | kFailed)) != 0;
        entity.m_runtime->m_prepared = ownPrepared && entity.m_runtime->m_unreadyChildren == 0;
        return entity.m_runtime->m_prepared;
    }


    bool EntityWorld::ActivateSubtree(Entity& entity)
    {
        if (!entity.m_wantsActive || entity.m_failed)
            return false;

        auto& chunk = *entity.m_chunk;
        ComponentContext context{ entity, *this, m_impl->m_services };
        for (const uint32_t column : chunk.m_archetype.m_lifecycleOrder)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            if (!(stage & kInitialized) || (stage & kActive))
                continue;

            const auto& info = *chunk.m_archetype.m_columns[column];
            stage |= kActive;
            MarkChanged(entity);
            if (info.m_activate && info.m_activate(chunk.Get(entity.m_row, column), context) != LifecycleResult::kSucceeded)
            {
                if (entity.m_active)
                {
                    TeardownComponent(entity, column, false);
                    chunk.Stage(entity.m_row, column) = kFailed;
                }
                else
                    entity.m_failed = true;
                return false;
            }
        }

        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            if ((child->m_active || child->m_runtime->m_prepared) && !ActivateSubtree(*child))
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);
                return false;
            }
        }
        return true;
    }


    void EntityWorld::AdvanceReplacements(Entity& entity)
    {
        if (!entity.m_active || !entity.m_wantsActive)
            return;

        auto& replacements = entity.m_runtime->m_replacements;
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
            ComponentContext context{ entity, *this, m_impl->m_services };
            if (result == LifecycleResult::kSucceeded)
            {
                replacement.m_stage |= kInitialized;
                if (info.m_init)
                    result = info.m_init(replacement.m_data, context);
            }
            if (result == LifecycleResult::kSucceeded)
            {
                replacement.m_stage |= kActive;
                MarkChanged(entity);
                if (info.m_activate)
                    result = info.m_activate(replacement.m_data, context);
            }
            if (result == LifecycleResult::kSucceeded)
            {
                const uint32_t column = entity.m_chunk->m_archetype.Find(info.m_type->m_id);
                TeardownComponent(entity, column, true);
                info.m_type->m_moveConstructor(entity.m_chunk->Get(entity.m_row, column), replacement.m_data);
                entity.m_chunk->Stage(entity.m_row, column) = replacement.m_stage;
                for (auto& contribution : entity.m_runtime->m_assets)
                {
                    if (contribution.m_transition == replacement.m_transition)
                        contribution.m_transition = 0;
                }
            }
            else
            {
                TeardownValue(entity, info, replacement.m_data, replacement.m_stage, replacement.m_transition);
                Fail("Component replacement failed; previous active value retained");
            }
            info.m_type->m_destructor(replacement.m_data);
            Memory::DefaultFree(replacement.m_data);
            it = replacements.erase(it);
        }
    }


    void EntityWorld::MarkUnready(Entity& entity)
    {
        for (Entity* current = &entity; current; current = current->m_parent)
            current->m_runtime->m_prepared = false;
    }


    void EntityWorld::UnwindSubtree(Entity& entity)
    {
        DeactivateSubtree(entity, false);
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
            UnwindSubtree(*child);
        const auto& order = entity.m_chunk->m_archetype.m_lifecycleOrder;
        for (uint32_t i = order.size(); i > 0; --i)
            TeardownComponent(entity, order[i - 1], false);
        entity.m_failed = true;
        entity.m_runtime->m_prepared = false;
    }


    void EntityWorld::AdvanceLifecycle()
    {
        for (auto& slot : m_impl->m_slots)
        {
            if (slot.m_entity)
                AdvanceReplacements(*slot.m_entity);
        }


        auto publish = [&](auto&& self, Entity& entity) -> void {
            entity.m_active = true;
            MarkChanged(entity);
            for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
            {
                if (child->m_runtime->m_prepared && child->m_wantsActive && !child->m_failed)
                    self(self, *child);
            }
        };
        for (auto& slot : m_impl->m_slots)
        {
            Entity* entity = slot.m_entity;
            if (!entity || entity->m_parent || !entity->m_wantsActive || entity->m_failed)
                continue;

            const bool wasActive = entity->m_active;
            if (wasActive && entity->m_runtime->m_prepared)
                continue;

            PrepareSubtree(*entity);
            if (entity->m_failed)
            {
                if (!wasActive)
                    UnwindSubtree(*entity);
                continue;
            }
            if (entity->m_runtime->m_prepared || wasActive)
            {
                if (ActivateSubtree(*entity))
                    publish(publish, *entity);
                else if (!wasActive)
                    UnwindSubtree(*entity);
            }
        }
    }
} // namespace FE::Framework
