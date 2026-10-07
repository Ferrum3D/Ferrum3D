#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    void EntityWorld::AddSystem(WorldSystem& system)
    {
        FE_Assert(!m_impl->m_updating);
        FE_Assert(festd::find(m_impl->m_systems, &system) == m_impl->m_systems.end());
        m_impl->m_systems.push_back(&system);
        system.Init(*this);
    }


    void EntityWorld::RemoveSystem(WorldSystem& system)
    {
        FE_Assert(!m_impl->m_updating);
        const auto it = festd::find(m_impl->m_systems, &system);
        if (it != m_impl->m_systems.end())
        {
            system.Shutdown(*this);
            m_impl->m_systems.erase(it);
        }
    }


    void EntityWorld::BeginUpdate()
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_impl->m_updating);
        m_impl->ClearEpoch();
        m_impl->m_started = true;
        ++m_impl->m_epoch;
        Commit();

        m_impl->m_updating = true;
        m_impl->m_diagnostics = {};
        m_impl->m_conflicts.clear();
        m_impl->m_validated = false;
        m_impl->m_scheduleFailed = false;
        m_impl->m_collecting = true;

        auto restore = festd::defer([&] {
            m_impl->m_collecting = false;
        });
        for (auto* system : m_impl->m_systems)
        {
            EntityUpdateContext context{ *this, system, m_impl->m_epoch };
            system->Update(context);
        }
    }


    void* EntityWorld::AllocateTraversal(const size_t size, const size_t alignment)
    {
        FE_Assert(m_impl->m_updating && !m_impl->m_executing);
        void* storage = m_impl->m_epochArena.allocate(size, alignment);
        if (!storage)
        {
            storage = Memory::DefaultAllocate(size, alignment);
            m_impl->m_largeCaptures.push_back(storage);
        }

        return storage;
    }


    Rc<WaitGroup> EntityWorld::RecordTraversal(EntityUpdateContext& context, const Phase phase,
                                               const festd::span<const QueryAccess> accesses, void* callable,
                                               void (*invoke)(void*, Entity&, void**), void (*destroy)(void*),
                                               const festd::span<const Rc<WaitGroup>> prerequisites, const ExecutionPolicy policy,
                                               const bool cascade, ChangeCursor* cursor)
    {
        FE_Assert(&context.m_world == this && context.m_epoch == m_impl->m_epoch && m_impl->m_updating);
        FE_Assert(!m_impl->m_executing && !m_impl->m_validated);
        Traversal traversal;
        traversal.m_phase = phase;
        traversal.m_policy = policy;
        traversal.m_cascade = cascade;
        traversal.m_cursor = cursor;
        traversal.m_accesses.assign(accesses.begin(), accesses.end());
        traversal.m_callable = callable;
        traversal.m_invoke = invoke;
        traversal.m_destroy = destroy;
        traversal.m_system = context.m_system;
        traversal.m_completion = Rc<WaitGroup>(WaitGroup::Create(1));
        traversal.m_prerequisites.assign(prerequisites.begin(), prerequisites.end());

        Rc<WaitGroup> result = traversal.m_completion;

        m_impl->m_traversals.push_back(std::move(traversal));
        return result;
    }


    Rc<WaitGroup> EntityWorld::RecordStage(const Phase stage, void* callable, void (*invoke)(void*), void (*destroy)(void*),
                                           const festd::span<const Rc<WaitGroup>> prerequisites)
    {
        EntityUpdateContext context{ *this, nullptr, GetEpoch() };
        auto completion = RecordTraversal(context, stage, {}, callable, nullptr, destroy, prerequisites);
        m_impl->m_traversals.back().m_stageInvoke = invoke;
        return completion;
    }


    bool EntityWorld::AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite)
    {
        if (!m_impl->m_updating || m_impl->m_validated || !prerequisite)
            return Fail("Prerequisite edits require an unvalidated epoch and a valid group");

        for (auto& traversal : m_impl->m_traversals)
        {
            if (traversal.m_completion.Get() == &completion)
            {
                traversal.m_prerequisites.push_back(prerequisite);
                return true;
            }
        }

        return Fail("Completion does not belong to this epoch");
    }


    bool EntityWorld::SchedulePhase(const Phase phase, const festd::span<const Rc<WaitGroup>> prerequisites)
    {
        if (!m_impl->m_updating || m_impl->m_validated || m_impl->m_executing)
            return Fail("Phase scheduling requires an unvalidated update epoch");

        for (const auto& scheduled : m_impl->m_phases)
        {
            if (scheduled.m_phase == phase)
            {
                m_impl->m_scheduleFailed = true;
                return Fail("Phase scheduled twice in one epoch");
            }
        }

        ScheduledPhase scheduled{ phase };
        scheduled.m_prerequisites.assign(prerequisites.begin(), prerequisites.end());
        m_impl->m_phases.push_back(std::move(scheduled));
        return true;
    }


    bool EntityWorld::ValidateSchedule()
    {
        if (m_impl->m_validated)
            return true;

        if (!m_impl->m_updating || m_impl->m_scheduleFailed)
            return Fail("Invalid update schedule");

        auto phaseIndex = [&](Phase phase) {
            for (uint32_t i = 0; i < m_impl->m_phases.size(); ++i)
            {
                if (m_impl->m_phases[i].m_phase == phase)
                    return i;
            }
            return kInvalidIndex;
        };

        auto producerIndex = [&](const Rc<WaitGroup>& group) {
            for (uint32_t i = 0; i < m_impl->m_traversals.size(); ++i)
            {
                if (m_impl->m_traversals[i].m_completion.Get() == group.Get())
                    return i;
            }
            return kInvalidIndex;
        };

        for (const auto& traversal : m_impl->m_traversals)
        {
            if (traversal.m_cascade && traversal.m_policy == ExecutionPolicy::kParallelChunks)
                return Fail("Cascade queries cannot use ParallelChunks");

            if (!traversal.m_cascade && traversal.m_policy == ExecutionPolicy::kParallelHierarchyTrees)
                return Fail("ParallelHierarchyTrees requires a cascade query");

            const uint32_t phase = phaseIndex(traversal.m_phase);
            if (phase == kInvalidIndex)
                return Fail("A submitted traversal phase was omitted");

            for (const auto& access : traversal.m_accesses)
            {
                if (access.m_parent && traversal.m_policy == ExecutionPolicy::kParallelChunks)
                {
                    for (const auto& other : traversal.m_accesses)
                    {
                        if (!other.m_parent && other.m_write && other.m_type == access.m_type)
                            return Fail("Parallel parent reads alias self writes; use a cascade query");
                    }
                }

                if (!access.m_type.IsValid() || !Components().Find(access.m_type))
                    return Fail("Query references an unregistered component type");
            }

            for (const auto& group : traversal.m_prerequisites)
            {
                if (!group)
                    return Fail("Null traversal prerequisite");

                const uint32_t producer = producerIndex(group);
                if (producer != kInvalidIndex && phaseIndex(m_impl->m_traversals[producer].m_phase) > phase)
                    return Fail("Traversal depends on a later phase");
            }
        }

        for (uint32_t i = 0; i < m_impl->m_phases.size(); ++i)
        {
            for (const auto& group : m_impl->m_phases[i].m_prerequisites)
            {
                if (!group)
                    return Fail("Null phase prerequisite");

                const uint32_t producer = producerIndex(group);
                if (producer != kInvalidIndex && phaseIndex(m_impl->m_traversals[producer].m_phase) >= i)
                    return Fail("Phase prerequisite depends on itself or a later phase");
            }
        }

        // Validate the framework graph without waiting on deferred completions.
        festd::vector<bool> visited(m_impl->m_traversals.size(), false);
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
                for (const auto& group : m_impl->m_traversals[i].m_prerequisites)
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
                return Fail("Framework traversal dependency cycle");
        }

        for (auto& traversal : m_impl->m_traversals)
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
                auto& before = m_impl->m_traversals[order[a]];
                auto& after = m_impl->m_traversals[order[b]];
                if (before.m_cursor && before.m_cursor == after.m_cursor)
                    return Fail("A change cursor may be consumed only once per epoch");

                if (!(before.m_phase == after.m_phase))
                    continue;

                const bool stageExclusion = before.m_stageInvoke || after.m_stageInvoke;
                bool conflict = false;
                for (const auto& lhs : before.m_accesses)
                {
                    for (const auto& rhs : after.m_accesses)
                    {
                        if (lhs.m_type == rhs.m_type && (lhs.m_write || rhs.m_write))
                        {
                            conflict = true;
                            m_impl->m_conflicts.push_back({ lhs.m_type, order[a], order[b] });
                        }
                    }
                }

                const bool systemExclusion = before.m_system && before.m_system == after.m_system
                    && (before.m_policy == ExecutionPolicy::kSequential || after.m_policy == ExecutionPolicy::kSequential);

                if (conflict || systemExclusion || stageExclusion)
                {
                    after.m_dependencies.push_back(order[a]);
                    m_impl->m_diagnostics.m_conflictEdges += conflict;
                    m_impl->m_diagnostics.m_systemEdges += systemExclusion;
                    m_impl->m_diagnostics.m_stageEdges += stageExclusion;
                }
            }
        }

        m_impl->m_validated = true;
        return true;
    }


    bool EntityWorld::ExecuteSchedule()
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        if (!m_impl->m_validated && !ValidateSchedule())
            return false;

        FE_Assert(!m_impl->m_executing);
        m_impl->m_executing = true;
        auto restore = festd::defer([&] {
            m_impl->m_executing = false;
        });

        for (const auto& phase : m_impl->m_phases)
        {
            for (const auto& group : phase.m_prerequisites)
                group->Wait();

            for (uint32_t i = 0; i < m_impl->m_traversals.size(); ++i)
            {
                auto& traversal = m_impl->m_traversals[i];
                if (!(traversal.m_phase == phase.m_phase) || traversal.m_executed)
                    continue;

                festd::inline_vector<WaitGroup*> prerequisites;
                for (const auto& group : traversal.m_prerequisites)
                    prerequisites.push_back(group.Get());

                for (const uint32_t dependency : traversal.m_dependencies)
                    prerequisites.push_back(m_impl->m_traversals[dependency].m_completion.Get());

                const auto affinity =
                    traversal.m_stageInvoke ? Jobs::FiberAffinityMask::kMainThread : Jobs::FiberAffinityMask::kAllForeground;

                Jobs::Dispatch(affinity, prerequisites, Jobs::Priority::kNormal, [this, i] {
                    ExecuteTraversal(i);
                });

                {
                    std::lock_guard guard(m_impl->m_versionLock);
                    ++m_impl->m_diagnostics.m_jobs;
                }
            }

            for (const auto& traversal : m_impl->m_traversals)
            {
                if (traversal.m_phase == phase.m_phase)
                    traversal.m_completion->Wait();
            }
        }

        return true;
    }


    void EntityWorld::ExecuteTraversal(const uint32_t index)
    {
        FE_PROFILER_ZONE();

        auto& traversal = m_impl->m_traversals[index];
        if (traversal.m_stageInvoke)
        {
            m_impl->m_executing = false;
            traversal.m_stageInvoke(traversal.m_callable);
            m_impl->m_executing = true;
            traversal.m_executed = true;
            traversal.m_completion->Signal();
            return;
        }

        festd::vector<ArchetypeChunk*> chunks;
        bool cascadeChanged = false;
        {
            std::lock_guard guard(m_impl->m_versionLock);
            bool parentChanged = false;
            if (traversal.m_cursor)
            {
                for (const auto* chunk : m_impl->m_chunks)
                {
                    for (const auto& access : traversal.m_accesses)
                    {
                        const uint32_t column = chunk->m_archetype.Find(access.m_type);
                        if (access.m_parent && column != kInvalidIndex)
                            parentChanged |= chunk->m_versions[column] > traversal.m_cursor->m_version;
                    }
                }
            }

            for (auto* chunk : m_impl->m_chunks)
            {
                bool matches = true;
                bool changed = parentChanged || !traversal.m_cursor
                    || traversal.m_cursor->m_structureRevision != m_impl->m_structureRevision
                    || traversal.m_cursor->m_versionEra != m_impl->m_changeEra;

                for (const auto& access : traversal.m_accesses)
                {
                    if (access.m_parent)
                        continue;

                    const uint32_t column = chunk->m_archetype.Find(access.m_type);
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

        if (traversal.m_cascade && traversal.m_cursor)
        {
            cascadeChanged |= traversal.m_cursor->m_hierarchyRevision != m_impl->m_hierarchyRevision;
            if (!cascadeChanged)
                chunks.clear();
        }

        // Mappings contain only column indices. Row and parent locations are always resolved afresh.
        for (auto* chunk : chunks)
        {
            auto mapping = festd::find_if(traversal.m_mappings.begin(), traversal.m_mappings.end(), [&](const auto& entry) {
                return entry.m_archetype == &chunk->m_archetype;
            });
            if (mapping != traversal.m_mappings.end())
                continue;

            Traversal::Mapping entry{ &chunk->m_archetype };
            for (const auto& access : traversal.m_accesses)
                entry.m_columns.push_back(access.m_parent ? kInvalidIndex : chunk->m_archetype.Find(access.m_type));
            traversal.m_mappings.push_back(std::move(entry));
        }

        std::atomic<uint32_t> callbacks = 0;
        auto invoke = [&](Entity& entity) {
            if (!entity.m_active || festd::find(chunks, entity.m_chunk) == chunks.end())
                return;

            const auto mapping = festd::find_if(traversal.m_mappings.begin(), traversal.m_mappings.end(), [&](const auto& entry) {
                return entry.m_archetype == &entity.m_chunk->m_archetype;
            });

            festd::inline_vector<void*> values;
            for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
            {
                const auto& access = traversal.m_accesses[i];
                Entity* source = access.m_parent ? entity.m_parent : &entity;
                uint32_t column = mapping->m_columns[i];
                if (access.m_parent && source && source->m_chunk)
                    column = source->m_chunk->m_archetype.Find(access.m_type);

                void* value = source && source->m_active && column != kInvalidIndex
                        && (source->m_chunk->Stage(source->m_row, column) & kActive)
                    ? source->m_chunk->Get(source->m_row, column)
                    : nullptr;

                if (!value && !access.m_optional)
                    return;

                values.push_back(value);
            }

            auto& fiber = Threading::FiberRuntimeInfo::Get();
            CallbackContext callback{ this, &traversal, &entity };
            void* previous = fiber.m_executionContext;
            fiber.m_executionContext = &callback;
            auto restore = festd::defer([&] {
                fiber.m_executionContext = previous;
            });

            traversal.m_invoke(traversal.m_callable, entity, values.data());
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
                entity = entity == root ? nullptr : entity->m_nextSibling;
            }
        };

        Rc<WaitGroup> work = WaitGroup::Create();
        uint32_t jobs = 0;
        uint32_t batches = 0;
        if (traversal.m_cascade && !chunks.empty())
        {
            festd::vector<Entity*> roots;
            for (const auto& slot : m_impl->m_slots)
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

            work->Signal();
            work->Wait();
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

            work->Signal();
            work->Wait();
        }
        {
            std::lock_guard guard(m_impl->m_versionLock);
            const uint64_t publishedVersion = NextChangeVersion();
            for (auto* chunk : chunks)
            {
                for (const auto& access : traversal.m_accesses)
                {
                    const uint32_t column = chunk->m_archetype.Find(access.m_type);
                    if (access.m_write && !access.m_parent && column != kInvalidIndex)
                        chunk->m_versions[column] = publishedVersion;
                }
            }

            if (traversal.m_cursor)
            {
                traversal.m_cursor->m_version = publishedVersion;
                traversal.m_cursor->m_versionEra = m_impl->m_changeEra;
                traversal.m_cursor->m_hierarchyRevision = m_impl->m_hierarchyRevision;
                traversal.m_cursor->m_structureRevision = m_impl->m_structureRevision;
            }

            m_impl->m_diagnostics.m_jobs += jobs;
            m_impl->m_diagnostics.m_chunks += chunks.size();
            m_impl->m_diagnostics.m_treeBatches += batches;
            m_impl->m_diagnostics.m_callbacks += callbacks.load();
        }

        traversal.m_executed = true;
        traversal.m_completion->Signal();
    }


    ScheduleDiagnostics EntityWorld::GetScheduleDiagnostics() const
    {
        std::lock_guard guard(m_impl->m_versionLock);
        return m_impl->m_diagnostics;
    }


    festd::span<const ScheduleConflict> EntityWorld::GetScheduleConflicts() const
    {
        return m_impl->m_conflicts;
    }


    bool EntityWorld::EndUpdate()
    {
        if (!m_impl->m_updating)
            return Fail("No update epoch is open");

        bool completed = true;
        for (const auto& traversal : m_impl->m_traversals)
            completed &= traversal.m_executed;

        if (!completed)
            Fail("Update ended with unexecuted deferred work");

        m_impl->ClearEpoch();
        m_impl->m_updating = false;
        return completed;
    }
} // namespace FE::Framework
