#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldInternal.h>
#include <algorithm>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    void EntityScheduler::AddSystem(WorldSystem& system)
    {
        FE_Assert(Threading::IsMainThread() && !IsUpdating());
        FE_Assert(festd::find(m_systems, &system) == m_systems.end());
        m_systems.push_back(&system);
        system.Init(m_world.m_owner);
    }


    void EntityScheduler::RemoveSystem(WorldSystem& system)
    {
        FE_Assert(Threading::IsMainThread() && !IsUpdating());

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
        FE_Assert(!IsUpdating());
        ClearEpoch();
        m_world.m_started = true;
        ++m_epoch;
        for (WorldService* service : m_world.m_services)
            service->Update(m_world.m_owner);

        m_world.CommitImpl(false);

        m_state = State::kUpdating;
        m_diagnostics = {};
        m_conflicts.clear();
        m_validated = false;
        m_state = State::kCollecting;

        auto restore = festd::defer([&] {
            m_state = State::kUpdating;
        });
        for (auto* system : m_systems)
        {
            EntityUpdateContext context{ m_world.m_owner, system, m_epoch };
            system->Update(context);
        }
    }


    void* EntityScheduler::AllocateTraversal(const size_t size, const size_t alignment)
    {
        FE_Assert(IsUpdating() && !IsExecuting() && !m_validated);
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
        FE_Assert(&context.m_world == &m_world.m_owner && context.m_epoch == m_epoch && IsUpdating());
        FE_Assert(!IsExecuting() && !m_validated);
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
        FE_Assert(IsUpdating() && !m_validated);
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
        FE_Assert(IsUpdating() && !m_validated && !IsExecuting());

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

        FE_Assert(IsUpdating());
        festd::unordered_dense_map<uint64_t, uint32_t> phaseIndices;
        for (uint32_t i = 0; i < m_phases.size(); ++i)
        {
            phaseIndices.emplace(m_phases[i].m_phase.m_id, i);
            m_phases[i].m_traversals.clear();
        }
        auto phaseIndex = [&](Phase phase) {
            const auto found = phaseIndices.find(phase.m_id);
            return found == phaseIndices.end() ? kInvalidIndex : found->second;
        };

        festd::unordered_dense_map<WaitGroup*, uint32_t> producers;
        for (uint32_t i = 0; i < m_traversals.size(); ++i)
            producers.emplace(m_traversals[i].m_completion.Get(), i);

        auto producerIndex = [&](const Rc<WaitGroup>& group) {
            const auto found = producers.find(group.Get());
            return found == producers.end() ? kInvalidIndex : found->second;
        };

        auto addDependency = [](Traversal& traversal, uint32_t producer) {
            if (festd::find(traversal.m_dependencies, producer) == traversal.m_dependencies.end())
                traversal.m_dependencies.push_back(producer);
        };

        // Validate every access and phase before any callback is eligible for dispatch.
        for (uint32_t index = 0; index < m_traversals.size(); ++index)
        {
            auto& traversal = m_traversals[index];
            traversal.m_dependencies.clear();
            traversal.m_externalPrerequisites.clear();
            FE_Assert(traversal.m_invoke || traversal.m_stageInvoke, "Traversal requires an invocation adapter");

            const uint32_t phase = phaseIndex(traversal.m_phase);
            if (phase == kInvalidIndex)
            {
                FE_Assert(false, "A submitted traversal phase was omitted");
                return false;
            }

            m_phases[phase].m_traversals.push_back(index);
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

                if (producer != kInvalidIndex)
                    addDependency(traversal, producer);
                else if (festd::find(traversal.m_externalPrerequisites, group) == traversal.m_externalPrerequisites.end())
                    traversal.m_externalPrerequisites.push_back(group);
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

        // Normalize the internal graph once; external wait groups have no producer in this epoch.
        festd::vector<uint32_t> indegrees(m_traversals.size(), 0);
        festd::vector<festd::inline_vector<uint32_t>> dependents(m_traversals.size());
        festd::vector<uint32_t> order;

        struct ReadyTraversal
        {
            uint32_t m_pass;
            uint32_t m_index;
        };

        festd::vector<ReadyTraversal> ready;
        auto later = [](const ReadyTraversal& lhs, const ReadyTraversal& rhs) {
            return lhs.m_pass != rhs.m_pass ? lhs.m_pass > rhs.m_pass : lhs.m_index > rhs.m_index;
        };
        for (uint32_t i = 0; i < m_traversals.size(); ++i)
        {
            indegrees[i] = m_traversals[i].m_dependencies.size();
            for (uint32_t producer : m_traversals[i].m_dependencies)
                dependents[producer].push_back(i);

            if (indegrees[i] == 0)
                ready.push_back({ 0, i });
        }

        // Preserve recording-order scans: an earlier newly ready traversal belongs to the next pass.
        std::make_heap(ready.begin(), ready.end(), later);
        while (!ready.empty())
        {
            std::pop_heap(ready.begin(), ready.end(), later);
            const ReadyTraversal current = ready.back();
            ready.pop_back();
            order.push_back(current.m_index);
            for (uint32_t dependent : dependents[current.m_index])
            {
                if (--indegrees[dependent] == 0)
                {
                    ready.push_back({ current.m_pass + (dependent < current.m_index), dependent });
                    std::push_heap(ready.begin(), ready.end(), later);
                }
            }
        }
        if (order.size() != m_traversals.size())
        {
            FE_Assert(false, "Framework traversal dependency cycle");
            return false;
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
                    && (before.m_policy != ExecutionPolicy::kParallel || after.m_policy != ExecutionPolicy::kParallel);

                if (conflict || systemExclusion || stageExclusion)
                {
                    addDependency(after, order[a]);
                    m_diagnostics.m_conflictEdges += conflict;
                    m_diagnostics.m_systemEdges += systemExclusion;
                    m_diagnostics.m_stageEdges += stageExclusion;
                }
            }
        }

        for (auto& traversal : m_traversals)
            traversal.m_prerequisites.clear();

        m_validated = true;
        return true;
    }


    bool EntityScheduler::ExecuteSchedule()
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        if (!m_validated && !ValidateSchedule())
            return false;

        FE_Assert(IsUpdating() && !IsExecuting() && !IsCollecting());
        m_state = State::kExecuting;

        auto restore = festd::defer([&] {
            m_state = State::kUpdating;
        });

        for (const auto& phase : m_phases)
        {
            for (const auto& group : phase.m_prerequisites)
                group->Wait();

            for (const uint32_t i : phase.m_traversals)
            {
                auto& traversal = m_traversals[i];
                if (traversal.m_executed)
                    continue;

                festd::inline_vector<WaitGroup*> prerequisites;
                for (const auto& group : traversal.m_externalPrerequisites)
                    prerequisites.push_back(group.Get());

                for (const uint32_t dependency : traversal.m_dependencies)
                    prerequisites.push_back(m_traversals[dependency].m_completion.Get());

                const bool mainThread = traversal.m_stageInvoke || traversal.m_policy == ExecutionPolicy::kMainThread;
                const auto affinity = mainThread ? Jobs::FiberAffinityMask::kMainThread : Jobs::FiberAffinityMask::kAllForeground;

                Jobs::Dispatch(affinity, prerequisites, Jobs::Priority::kNormal, [this, i] {
                    ExecuteTraversal(i);
                });

                {
                    std::lock_guard guard(m_world.m_storage.m_versionLock);
                    ++m_diagnostics.m_jobs;
                }
            }

            for (const uint32_t index : phase.m_traversals)
                m_traversals[index].m_completion->Wait();
        }

        return true;
    }


    void EntityScheduler::PrepareTraversal(const Traversal& traversal, festd::vector<PreparedChunk>& chunks)
    {
        // Layout matching does not access versions or mutable component values.
        for (auto* chunk : m_world.m_storage.m_chunks)
        {
            PreparedChunk entry{ chunk };
            bool matches = true;
            for (const auto& access : traversal.m_accesses)
            {
                const uint32_t column = chunk->m_archetype.Find(access.m_type);
                entry.m_columns.push_back(column);
                if (access.m_parent)
                    continue;

                if (access.m_excluded)
                    matches &= column == kInvalidIndex;
                else if (!access.m_optional)
                    matches &= column != kInvalidIndex;
            }

            if (matches)
                chunks.push_back(std::move(entry));
        }

        if (!traversal.m_cursor)
            return;

        // Parent terms may invalidate matching children even when the parent's own archetype is excluded.
        festd::inline_vector<std::pair<const ArchetypeChunk*, uint32_t>> parentColumns;
        for (const auto& access : traversal.m_accesses)
        {
            if (!access.m_parent)
                continue;

            for (const auto* chunk : m_world.m_storage.m_chunks)
            {
                const uint32_t column = chunk->m_archetype.Find(access.m_type);
                if (column != kInvalidIndex)
                    parentColumns.emplace_back(chunk, column);
            }
        }

        std::lock_guard guard(m_world.m_storage.m_versionLock);
        const auto& cursor = *traversal.m_cursor;
        const bool invalidated = cursor.m_structureRevision != m_world.m_storage.m_structureRevision
            || cursor.m_versionEra != m_world.m_storage.m_changeEra;
        bool parentChanged = false;
        for (const auto& [chunk, column] : parentColumns)
            parentChanged |= chunk->m_versions[column] > cursor.m_version;

        auto changed = [&](const PreparedChunk& entry) {
            if (invalidated || parentChanged)
                return true;

            for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
            {
                const auto& access = traversal.m_accesses[i];
                const uint32_t column = entry.m_columns[i];
                if (!access.m_parent && !access.m_excluded && column != kInvalidIndex
                    && entry.m_chunk->m_versions[column] > cursor.m_version)
                {
                    return true;
                }
            }

            return false;
        };

        if (traversal.m_cascade)
        {
            // Reprocess all matching descendants, including through filtered ancestors.
            const bool hierarchyChanged = cursor.m_hierarchyRevision != m_world.m_storage.m_hierarchyRevision;
            if (!hierarchyChanged && !std::any_of(chunks.begin(), chunks.end(), changed))
                chunks.clear();
        }
        else
        {
            chunks.erase(std::remove_if(chunks.begin(),
                                        chunks.end(),
                                        [&](const PreparedChunk& entry) {
                                            return !changed(entry);
                                        }),
                         chunks.end());
        }
    }


    bool EntityScheduler::InvokeEntity(const Traversal& traversal, Entity& entity, const PreparedChunk& chunk,
                                       festd::span<void*> values)
    {
        if (!entity.m_active)
            return false;

        for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
        {
            const auto& access = traversal.m_accesses[i];
            if (access.m_excluded)
                continue;

            Entity* source = access.m_parent ? entity.m_parent.Get() : &entity;
            uint32_t column = chunk.m_columns[i];
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
            // A structural stage may commit queued edits, but cannot record work into the compiled schedule.
            m_state = State::kStructuralStage;
            traversal.m_stageInvoke(traversal.m_callable);
            m_state = State::kExecuting;
            traversal.m_executed = true;
            traversal.m_completion->Signal();
            return;
        }

        festd::vector<PreparedChunk> chunks;
        PrepareTraversal(traversal, chunks);

        // Ordinary row loops already know their mapping; hierarchy walks need one lookup per visited entity.
        festd::unordered_dense_map<const ArchetypeChunk*, uint32_t> chunkLookup;
        if (traversal.m_cascade)
        {
            for (uint32_t i = 0; i < chunks.size(); ++i)
                chunkLookup.emplace(chunks[i].m_chunk, i);
        }

        std::atomic<uint32_t> callbacks = 0;
        auto visitTree = [&](Entity* root, festd::span<void*> values) {
            uint32_t count = 0;
            Entity* entity = root;
            while (entity)
            {
                const auto found = chunkLookup.find(entity->m_chunk);
                if (found != chunkLookup.end() && InvokeEntity(traversal, *entity, chunks[found->second], values))
                    ++count;

                if (entity->m_firstChild)
                {
                    entity = entity->m_firstChild;
                    continue;
                }

                while (entity != root && !entity->m_nextSibling)
                    entity = entity->m_parent;

                entity = entity == root ? nullptr : entity->m_nextSibling.Get();
            }

            return count;
        };

        uint32_t jobs = 0;
        uint32_t batches = 0;
        Rc<WaitGroup> work;
        const bool parallel = traversal.m_policy == ExecutionPolicy::kParallel;
        if (parallel && !chunks.empty())
            work = WaitGroup::Create();

        festd::vector<Entity*> roots;
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
                auto visitBatch = [&, begin, end] {
                    festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
                    uint32_t count = 0;
                    for (uint32_t i = begin; i < end; ++i)
                        count += visitTree(roots[i], values);

                    callbacks.fetch_add(count, std::memory_order_relaxed);
                };
                if (!parallel)
                {
                    visitBatch();
                }
                else
                {
                    work->Add(1);
                    ++jobs;
                    ++batches;
                    Jobs::DispatchForeground(std::move(visitBatch), work.Get());
                }
            }
        }
        else
        {
            for (const auto& chunk : chunks)
            {
                auto visitChunk = [&, entry = &chunk] {
                    festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
                    uint32_t count = 0;
                    for (uint32_t row = 0; row < entry->m_chunk->m_count; ++row)
                    {
                        if (InvokeEntity(traversal, *entry->m_chunk->m_entities[row], *entry, values))
                            ++count;
                    }

                    callbacks.fetch_add(count, std::memory_order_relaxed);
                };
                if (!parallel)
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
        if (work)
        {
            work->Signal();
            work->Wait();
        }

        {
            std::lock_guard guard(m_world.m_storage.m_versionLock);

            const bool writes =
                std::any_of(traversal.m_accesses.begin(), traversal.m_accesses.end(), [](const QueryAccess& access) {
                    return access.m_write && !access.m_parent && !access.m_excluded;
                });
            const uint64_t publishedVersion = writes ? m_world.NextChangeVersion() : m_world.m_storage.m_changeVersion;
            if (writes)
            {
                for (const auto& entry : chunks)
                {
                    for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
                    {
                        const auto& access = traversal.m_accesses[i];
                        const uint32_t column = entry.m_columns[i];
                        if (access.m_write && !access.m_parent && !access.m_excluded && column != kInvalidIndex)
                            entry.m_chunk->m_versions[column] = publishedVersion;
                    }
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
        FE_Assert(Threading::IsMainThread() && IsUpdating() && !IsExecuting() && !IsCollecting());
        auto cleanup = festd::defer([&] {
            ClearEpoch();
            m_state = State::kIdle;
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
        FE_Assert(!IsExecuting() && !IsCollecting());
        ClearEpoch();
        for (uint32_t i = m_systems.size(); i > 0; --i)
            m_systems[i - 1]->Shutdown(m_world.m_owner);

        m_systems.clear();
    }
} // namespace FE::Framework
