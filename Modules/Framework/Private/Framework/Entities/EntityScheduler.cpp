#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldInternal.h>

namespace FE::Framework
{
    void EntityScheduler::AddSystem(WorldSystem& system)
    {
        FE_Assert(Threading::IsMainThread() && !m_updating);
        FE_Assert(festd::find(m_systems, &system) == m_systems.end());
        m_systems.push_back(&system);
        system.Init(m_world.m_owner);
    }


    void EntityScheduler::RemoveSystem(WorldSystem& system)
    {
        FE_Assert(Threading::IsMainThread() && !m_updating);

        const auto it = festd::find(m_systems, &system);
        if (it != m_systems.end())
        {
            system.Shutdown(m_world.m_owner);
            m_systems.erase(it);
        }
    }


    void EntityScheduler::BeginUpdate()
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_updating);
        ClearEpoch();
        m_world.m_started = true;
        ++m_epoch;
        for (WorldService* service : m_world.m_services)
            service->Update(m_world.m_owner);

        m_world.CommitImpl(false);

        m_updating = true;
        m_diagnostics = {};
        m_conflicts.clear();
        m_validated = false;
        m_collecting = true;

        auto restore = festd::defer([&] {
            m_collecting = false;
        });
        for (auto* system : m_systems)
        {
            EntityUpdateContext context{ m_world.m_owner, system, m_epoch };
            system->Update(context);
        }
    }


    void* EntityScheduler::AllocateTraversal(const size_t size, const size_t alignment)
    {
        FE_Assert(m_updating && !m_executing && !m_validated);
        void* storage = m_epochArena.allocate(size, alignment);
        if (!storage)
        {
            storage = Memory::DefaultAllocate(size, alignment);
            m_largeCaptures.push_back(storage);
        }

        return storage;
    }


    Rc<WaitGroup> EntityScheduler::RecordTraversal(EntityUpdateContext& context, const TraversalDesc& desc)
    {
        FE_Assert(desc.m_callable && desc.m_destroy);
        FE_Assert(&context.m_world == &m_world.m_owner && context.m_epoch == m_epoch && m_updating);
        FE_Assert(!m_executing && !m_validated);
        Traversal traversal;
        traversal.m_phase = desc.m_phase;
        traversal.m_policy = desc.m_policy;
        traversal.m_cascade = desc.m_cascade;
        traversal.m_cursor = desc.m_cursor;
        traversal.m_accesses.assign(desc.m_accesses.begin(), desc.m_accesses.end());
        traversal.m_callable = desc.m_callable;
        traversal.m_invoke = desc.m_invoke;
        traversal.m_destroy = desc.m_destroy;
        traversal.m_system = context.m_system;
        traversal.m_completion = Rc<WaitGroup>(WaitGroup::Create(1));
        traversal.m_prerequisites.assign(desc.m_prerequisites.begin(), desc.m_prerequisites.end());

        Rc<WaitGroup> result = traversal.m_completion;

        m_traversals.push_back(std::move(traversal));
        return result;
    }


    Rc<WaitGroup> EntityScheduler::RecordStage(const StageDesc& desc)
    {
        FE_Assert(desc.m_invoke);

        EntityUpdateContext context{ m_world.m_owner, nullptr, m_epoch };
        TraversalDesc traversal{ desc.m_phase, {}, desc.m_callable, nullptr, desc.m_destroy, desc.m_prerequisites };
        auto completion = RecordTraversal(context, traversal);
        m_traversals.back().m_stageInvoke = desc.m_invoke;
        return completion;
    }


    bool EntityScheduler::AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite)
    {
        FE_Assert(m_updating && !m_validated);
        if (!prerequisite)
        {
            FE_Assert(false, "Invalid schedule prerequisite");
            return false;
        }

        for (auto& traversal : m_traversals)
        {
            if (traversal.m_completion.Get() == &completion)
            {
                traversal.m_prerequisites.push_back(prerequisite);
                return true;
            }
        }

        FE_Assert(false, "Completion does not belong to this epoch");
        return false;
    }


    bool EntityScheduler::SchedulePhase(const Phase phase, const festd::span<const Rc<WaitGroup>> prerequisites)
    {
        FE_Assert(m_updating && !m_validated && !m_executing);

        for (const auto& scheduled : m_phases)
        {
            if (scheduled.m_phase == phase)
            {
                FE_Assert(false, "Phase scheduled twice in one epoch");
                return false;
            }
        }

        ScheduledPhase scheduled{ phase };
        scheduled.m_prerequisites.assign(prerequisites.begin(), prerequisites.end());
        m_phases.push_back(std::move(scheduled));
        return true;
    }


    bool EntityScheduler::ValidateSchedule()
    {
        FE_PROFILER_ZONE();

        if (m_validated)
            return true;

        FE_Assert(m_updating);
        auto phaseIndex = [&](Phase phase) {
            for (uint32_t i = 0; i < m_phases.size(); ++i)
            {
                if (m_phases[i].m_phase == phase)
                    return i;
            }
            return kInvalidIndex;
        };

        auto producerIndex = [&](const Rc<WaitGroup>& group) {
            for (uint32_t i = 0; i < m_traversals.size(); ++i)
            {
                if (m_traversals[i].m_completion.Get() == group.Get())
                    return i;
            }
            return kInvalidIndex;
        };

        // Validate every access and phase before any callback is eligible for dispatch.
        for (const auto& traversal : m_traversals)
        {
            FE_Assert(traversal.m_invoke || traversal.m_stageInvoke, "Traversal requires an invocation adapter");

            const uint32_t phase = phaseIndex(traversal.m_phase);
            if (phase == kInvalidIndex)
            {
                FE_Assert(false, "A submitted traversal phase was omitted");
                return false;
            }

            for (const auto& access : traversal.m_accesses)
            {
                if (!traversal.m_cascade && access.m_parent && traversal.m_policy == ExecutionPolicy::kParallel)
                {
                    for (const auto& other : traversal.m_accesses)
                    {
                        if (!other.m_parent && other.m_write && other.m_type == access.m_type)
                        {
                            FE_Assert(false, "Parallel parent reads alias self writes; use a cascade query");
                            return false;
                        }
                    }
                }

                if (!access.m_type.IsValid() || !m_world.m_components.Find(access.m_type))
                {
                    FE_Assert(false, "Query references an unregistered component type");
                    return false;
                }
            }

            for (const auto& group : traversal.m_prerequisites)
            {
                if (!group)
                {
                    FE_Assert(false, "Null traversal prerequisite");
                    return false;
                }

                const uint32_t producer = producerIndex(group);
                if (producer != kInvalidIndex && phaseIndex(m_traversals[producer].m_phase) > phase)
                {
                    FE_Assert(false, "Traversal depends on a later phase");
                    return false;
                }
            }
        }

        for (uint32_t i = 0; i < m_phases.size(); ++i)
        {
            for (const auto& group : m_phases[i].m_prerequisites)
            {
                if (!group)
                {
                    FE_Assert(false, "Null phase prerequisite");
                    return false;
                }

                const uint32_t producer = producerIndex(group);
                if (producer != kInvalidIndex && phaseIndex(m_traversals[producer].m_phase) >= i)
                {
                    FE_Assert(false, "Phase prerequisite depends on itself or a later phase");
                    return false;
                }
            }
        }

        // Reject explicit cycles before adding access exclusions, so no submitted job can wait forever.
        // Validate the framework graph without waiting on deferred completions.
        festd::vector<bool> visited(m_traversals.size(), false);
        festd::vector<uint32_t> order;
        uint32_t count = 0;
        while (count < visited.size())
        {
            bool progress = false;
            for (uint32_t i = 0; i < visited.size(); ++i)
            {
                if (visited[i])
                    continue;

                bool ready = true;
                for (const auto& group : m_traversals[i].m_prerequisites)
                {
                    const uint32_t producer = producerIndex(group);
                    if (producer != kInvalidIndex && !visited[producer])
                        ready = false;
                }

                if (!ready)
                    continue;

                visited[i] = true;
                order.push_back(i);
                ++count;
                progress = true;
            }

            if (!progress)
            {
                FE_Assert(false, "Framework traversal dependency cycle");
                return false;
            }
        }

        for (auto& traversal : m_traversals)
        {
            traversal.m_dependencies.clear();
            for (const auto& group : traversal.m_prerequisites)
            {
                const uint32_t producer = producerIndex(group);
                if (producer != kInvalidIndex)
                    traversal.m_dependencies.push_back(producer);
            }
        }

        // Orient conservative exclusions using the already validated explicit topological order.
        for (uint32_t a = 0; a < order.size(); ++a)
        {
            for (uint32_t b = a + 1; b < order.size(); ++b)
            {
                auto& before = m_traversals[order[a]];
                auto& after = m_traversals[order[b]];
                if (before.m_cursor && before.m_cursor == after.m_cursor)
                {
                    FE_Assert(false, "A change cursor may be consumed only once per epoch");
                    return false;
                }

                if (!(before.m_phase == after.m_phase))
                    continue;

                const bool stageExclusion = before.m_stageInvoke || after.m_stageInvoke;
                bool conflict = false;
                for (const auto& lhs : before.m_accesses)
                {
                    for (const auto& rhs : after.m_accesses)
                    {
                        if (!lhs.m_excluded && !rhs.m_excluded && lhs.m_type == rhs.m_type && (lhs.m_write || rhs.m_write))
                        {
                            conflict = true;
                            m_conflicts.push_back({ lhs.m_type, order[a], order[b] });
                        }
                    }
                }

                const bool systemExclusion = before.m_system && before.m_system == after.m_system
                    && (before.m_policy == ExecutionPolicy::kSequential || after.m_policy == ExecutionPolicy::kSequential);

                if (conflict || systemExclusion || stageExclusion)
                {
                    after.m_dependencies.push_back(order[a]);
                    m_diagnostics.m_conflictEdges += conflict;
                    m_diagnostics.m_systemEdges += systemExclusion;
                    m_diagnostics.m_stageEdges += stageExclusion;
                }
            }
        }

        m_validated = true;
        return true;
    }


    bool EntityScheduler::ExecuteSchedule()
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        if (!m_validated && !ValidateSchedule())
            return false;

        FE_Assert(m_updating && !m_executing && !m_collecting);
        m_executing = true;

        auto restore = festd::defer([&] {
            m_executing = false;
        });

        for (const auto& phase : m_phases)
        {
            for (const auto& group : phase.m_prerequisites)
                group->Wait();

            for (uint32_t i = 0; i < m_traversals.size(); ++i)
            {
                auto& traversal = m_traversals[i];
                if (!(traversal.m_phase == phase.m_phase) || traversal.m_executed)
                    continue;

                festd::inline_vector<WaitGroup*> prerequisites;
                for (const auto& group : traversal.m_prerequisites)
                    prerequisites.push_back(group.Get());

                for (const uint32_t dependency : traversal.m_dependencies)
                    prerequisites.push_back(m_traversals[dependency].m_completion.Get());

                const auto affinity =
                    traversal.m_stageInvoke ? Jobs::FiberAffinityMask::kMainThread : Jobs::FiberAffinityMask::kAllForeground;

                Jobs::Dispatch(affinity, prerequisites, Jobs::Priority::kNormal, [this, i] {
                    ExecuteTraversal(i);
                });

                {
                    std::lock_guard guard(m_world.m_storage.m_versionLock);
                    ++m_diagnostics.m_jobs;
                }
            }

            for (const auto& traversal : m_traversals)
            {
                if (traversal.m_phase == phase.m_phase)
                    traversal.m_completion->Wait();
            }
        }

        return true;
    }


    void EntityScheduler::PrepareTraversal(Traversal& traversal, festd::vector<ArchetypeChunk*>& chunks)
    {
        bool cascadeChanged = false;
        {
            std::lock_guard guard(m_world.m_storage.m_versionLock);

            bool parentChanged = false;
            if (traversal.m_cursor)
            {
                for (const auto* chunk : m_world.m_storage.m_chunks)
                {
                    for (const auto& access : traversal.m_accesses)
                    {
                        if (!access.m_parent)
                            continue;

                        const uint32_t column = chunk->m_archetype.Find(access.m_type);
                        if (column != kInvalidIndex)
                            parentChanged |= chunk->m_versions[column] > traversal.m_cursor->m_version;
                    }
                }
            }

            for (auto* chunk : m_world.m_storage.m_chunks)
            {
                bool matches = true;
                bool changed = parentChanged || !traversal.m_cursor
                    || traversal.m_cursor->m_structureRevision != m_world.m_storage.m_structureRevision
                    || traversal.m_cursor->m_versionEra != m_world.m_storage.m_changeEra;

                for (const auto& access : traversal.m_accesses)
                {
                    if (access.m_parent)
                        continue;

                    const uint32_t column = chunk->m_archetype.Find(access.m_type);
                    if (access.m_excluded)
                    {
                        matches &= column == kInvalidIndex;
                        continue;
                    }

                    if (column == kInvalidIndex && !access.m_optional)
                        matches = false;

                    if (column != kInvalidIndex && traversal.m_cursor
                        && chunk->m_versions[column] > traversal.m_cursor->m_version)
                    {
                        changed = true;
                    }
                }

                if (matches)
                {
                    if (traversal.m_cascade || changed)
                        chunks.push_back(chunk);

                    cascadeChanged |= changed;
                }
            }
        }

        // Cascade changes conservatively reprocess matching descendants, including through filtered ancestors.
        if (traversal.m_cascade && traversal.m_cursor)
        {
            cascadeChanged |= traversal.m_cursor->m_hierarchyRevision != m_world.m_storage.m_hierarchyRevision;
            if (!cascadeChanged)
                chunks.clear();
        }

        // Mappings contain only column indices. Row and parent locations are always resolved afresh.
        for (auto* chunk : chunks)
        {
            auto mapping =
                festd::find_if(traversal.m_mappings.begin(), traversal.m_mappings.end(), [&](const Traversal::Mapping& entry) {
                    return entry.m_archetype == &chunk->m_archetype;
                });
            if (mapping != traversal.m_mappings.end())
                continue;

            Traversal::Mapping entry{ &chunk->m_archetype };
            for (const auto& access : traversal.m_accesses)
            {
                entry.m_columns.push_back(access.m_parent || access.m_excluded ? kInvalidIndex
                                                                               : chunk->m_archetype.Find(access.m_type));
            }
            traversal.m_mappings.push_back(std::move(entry));
        }
    }


    bool EntityScheduler::InvokeEntity(Traversal& traversal, Entity& entity, const festd::vector<ArchetypeChunk*>& chunks)
    {
        if (!entity.m_active || festd::find(chunks, entity.m_chunk) == chunks.end())
            return false;

        const auto mapping =
            festd::find_if(traversal.m_mappings.begin(), traversal.m_mappings.end(), [&](const Traversal::Mapping& entry) {
                return entry.m_archetype == &entity.m_chunk->m_archetype;
            });

        festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
        for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
        {
            const auto& access = traversal.m_accesses[i];
            if (access.m_excluded)
                continue;

            Entity* source = access.m_parent ? entity.m_parent.Get() : &entity;
            uint32_t column = mapping->m_columns[i];
            if (access.m_parent && source && source->m_chunk)
                column = source->m_chunk->m_archetype.Find(access.m_type);

            void* value = source && source->m_active && column != kInvalidIndex
                    && (source->m_chunk->Stage(source->m_row, column) & ComponentStage::kActive) != ComponentStage::kNone
                ? source->m_chunk->Get(source->m_row, column)
                : nullptr;

            if (!value && !access.m_optional)
                return false;

            values[i] = value;
        }

        auto& fiber = Threading::FiberRuntimeInfo::Get();
        CallbackContext callback{ &m_world.m_owner, &traversal, &entity };
        void* previous = fiber.m_executionContext;
        fiber.m_executionContext = &callback;

        auto restore = festd::defer([&] {
            fiber.m_executionContext = previous;
        });
        traversal.m_invoke(traversal.m_callable, entity, values.data());
        return true;
    }


    void EntityScheduler::ExecuteTraversal(const uint32_t index)
    {
        FE_PROFILER_ZONE();

        auto& traversal = m_traversals[index];
        if (traversal.m_stageInvoke)
        {
            m_executing = false;
            traversal.m_stageInvoke(traversal.m_callable);
            m_executing = true;
            traversal.m_executed = true;
            traversal.m_completion->Signal();
            return;
        }

        festd::vector<ArchetypeChunk*> chunks;
        PrepareTraversal(traversal, chunks);

        // Subdivide only substantial chunk/tree work; component callbacks keep the coarse traversal zone.
        std::atomic<uint32_t> callbacks = 0;
        auto invoke = [&](Entity& entity) {
            if (InvokeEntity(traversal, entity, chunks))
                callbacks.fetch_add(1, std::memory_order_relaxed);
        };

        auto visitTree = [&](Entity* root) {
            Entity* entity = root;
            while (entity)
            {
                invoke(*entity);
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

        Rc<WaitGroup> work = WaitGroup::Create();
        festd::vector<Entity*> roots;
        uint32_t jobs = 0;
        uint32_t batches = 0;
        if (traversal.m_cascade && !chunks.empty())
        {
            for (const auto& slot : m_world.m_storage.m_slots)
            {
                if (slot.m_entity && !slot.m_entity->m_parent)
                    roots.push_back(slot.m_entity);
            }

            constexpr uint32_t kTreesPerBatch = 16;
            for (uint32_t begin = 0; begin < roots.size(); begin += kTreesPerBatch)
            {
                const uint32_t end = Math::Min(begin + kTreesPerBatch, roots.size());
                if (traversal.m_policy == ExecutionPolicy::kSequential)
                {
                    for (uint32_t i = begin; i < end; ++i)
                        visitTree(roots[i]);
                }
                else
                {
                    work->Add(1);
                    ++jobs;
                    ++batches;
                    Jobs::DispatchForeground(
                        [&, begin, end] {
                            for (uint32_t i = begin; i < end; ++i)
                                visitTree(roots[i]);
                        },
                        work.Get());
                }
            }
        }
        else
        {
            for (auto* chunk : chunks)
            {
                auto visitChunk = [&, chunk] {
                    for (uint32_t row = 0; row < chunk->m_count; ++row)
                        invoke(*chunk->m_entities[row]);
                };

                if (traversal.m_policy == ExecutionPolicy::kSequential)
                {
                    visitChunk();
                }
                else
                {
                    work->Add(1);
                    ++jobs;
                    Jobs::DispatchForeground(std::move(visitChunk), work.Get());
                }
            }
        }
        work->Signal();
        work->Wait();

        {
            std::lock_guard guard(m_world.m_storage.m_versionLock);

            const uint64_t publishedVersion = m_world.NextChangeVersion();
            for (auto* chunk : chunks)
            {
                for (const auto& access : traversal.m_accesses)
                {
                    const uint32_t column = chunk->m_archetype.Find(access.m_type);
                    if (access.m_write && !access.m_parent && !access.m_excluded && column != kInvalidIndex)
                        chunk->m_versions[column] = publishedVersion;
                }
            }

            if (traversal.m_cursor)
            {
                traversal.m_cursor->m_version = publishedVersion;
                traversal.m_cursor->m_versionEra = m_world.m_storage.m_changeEra;
                traversal.m_cursor->m_hierarchyRevision = m_world.m_storage.m_hierarchyRevision;
                traversal.m_cursor->m_structureRevision = m_world.m_storage.m_structureRevision;
            }
            m_diagnostics.m_chunks += chunks.size();
            m_diagnostics.m_jobs += jobs;
            m_diagnostics.m_treeBatches += batches;
            m_diagnostics.m_callbacks += callbacks.load();
        }

        traversal.m_executed = true;
        traversal.m_completion->Signal();
    }


    ScheduleDiagnostics EntityScheduler::GetDiagnostics() const
    {
        std::lock_guard guard(m_world.m_storage.m_versionLock);
        return m_diagnostics;
    }


    festd::span<const ScheduleConflict> EntityScheduler::GetConflicts() const
    {
        return m_conflicts;
    }


    bool EntityScheduler::EndUpdate()
    {
        FE_Assert(Threading::IsMainThread() && m_updating && !m_executing && !m_collecting);
        auto cleanup = festd::defer([&] {
            ClearEpoch();
            m_updating = false;
        });

        bool completed = true;
        for (const auto& traversal : m_traversals)
            completed &= traversal.m_executed;

        FE_Assert(completed, "Incomplete update schedule");
        return completed;
    }


    void EntityScheduler::ClearEpoch()
    {
        for (auto& traversal : m_traversals)
        {
            // Wake dependents even when the epoch is abandoned during shutdown.
            if (!traversal.m_completion->IsSignaled())
                traversal.m_completion->Signal();

            traversal.m_destroy(traversal.m_callable);
        }

        m_traversals.clear();
        m_phases.clear();
        for (void* capture : m_largeCaptures)
            Memory::DefaultFree(capture);

        m_largeCaptures.clear();
        m_epochArena.Clear();
    }


    void EntityScheduler::Shutdown()
    {
        FE_Assert(!m_executing && !m_collecting);
        ClearEpoch();
        for (uint32_t i = m_systems.size(); i > 0; --i)
            m_systems[i - 1]->Shutdown(m_world.m_owner);

        m_systems.clear();
    }
} // namespace FE::Framework
