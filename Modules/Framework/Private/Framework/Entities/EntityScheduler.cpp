#include <Core/CLI/CommandLine.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Memory/FiberTempAllocator.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldInternal.h>
#include <Framework/Entities/WorldTimeService.h>
#include <algorithm>

namespace FE::Framework
{
    namespace
    {
        constexpr uint32_t kRowsPerBatch = 32;
        constexpr uint32_t kEntitiesPerTreeBatch = 16;


        //! @brief Report an invalid plan; validation asserts in every build, unvalidated compilation only in debug.
        bool Reject(const bool validate, const char* message)
        {
            if (validate)
            {
                Logger::LogError("Invalid ECS schedule: {}", message);
                FE_Assert(false, "Invalid ECS schedule");
            }
            else
            {
                FE_AssertDebug(false, "Invalid ECS schedule");
            }

            return false;
        }


        festd::ascii_view GetTraversalName(const Traversal& traversal)
        {
            return traversal.m_system ? traversal.m_system->RTTI_GetDynamicType().m_name : "Query";
        }


        //! @brief Main-thread or worker job for one traversal; lives in the epoch arena and is destroyed on completion.
        struct TraversalJob final : Jobs::JobNode
        {
            EntityScheduler* m_scheduler;
            void (EntityScheduler::*m_execute)(uint32_t);
            uint32_t m_index;

            TraversalJob(EntityScheduler& scheduler, void (EntityScheduler::*execute)(uint32_t), const uint32_t index)
                : m_scheduler(&scheduler)
                , m_execute(execute)
                , m_index(index)
            {
            }

            void Execute() override
            {
                (m_scheduler->*m_execute)(m_index);
                this->~TraversalJob();
            }
        };


        //! @brief Run visit(batch) for every batch, dispatching jobs from fiber-local memory when parallel work pays off.
        //! Return the number of dispatched jobs; zero when the batches ran inline.
        template<class TVisit>
        uint32_t RunBatches(const bool parallel, const uint32_t batchCount, std::pmr::memory_resource& temp, TVisit& visit)
        {
            if (!parallel || batchCount <= 1)
            {
                for (uint32_t batch = 0; batch < batchCount; ++batch)
                    visit(batch);

                return 0;
            }

            struct BatchJob final : Jobs::JobNode
            {
                TVisit* m_visit = nullptr;
                uint32_t m_batch = 0;

                void Execute() override
                {
                    (*m_visit)(m_batch);
                    this->~BatchJob();
                }
            };

            // Batch jobs and the joined completion outlive dispatch only until the wait below returns.
            auto* jobs = static_cast<BatchJob*>(temp.allocate(sizeof(BatchJob) * batchCount, alignof(BatchJob)));
            const Rc<WaitGroup> completion = WaitGroup::Create(batchCount + 1);
            for (uint32_t batch = 0; batch < batchCount; ++batch)
            {
                auto* job = ::new (&jobs[batch]) BatchJob;
                job->m_visit = &visit;
                job->m_batch = batch;
                job->DispatchForeground(completion.Get());
            }

            FE_PROFILER_ZONE_NAMED("JoinBatches");
            completion->Signal();
            completion->Wait();
            return batchCount;
        }
    } // namespace


    EntityScheduler::EntityScheduler(EntityWorld::Impl& world)
        : m_world(world)
        , m_validationEnabled(Cli::Check("--ecs-validation"))
    {
    }


    void EntityScheduler::AddSystem(WorldSystem& system)
    {
        FE_AssertDebug(Threading::IsMainThread() && !IsUpdating());
        FE_Assert(festd::find(m_systems, &system) == m_systems.end());
        m_systems.push_back(&system);
        system.Init(m_world.m_owner);
    }


    void EntityScheduler::RemoveSystem(WorldSystem& system)
    {
        FE_AssertDebug(Threading::IsMainThread() && !IsUpdating());

        const auto it = festd::find(m_systems, &system);
        if (it == m_systems.end())
            return;

        system.Shutdown(m_world.m_owner);
        m_systems.erase(it);
    }


    void EntityScheduler::BeginUpdate(const float fixedDeltaSeconds)
    {
        FE_PROFILER_ZONE();

        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_AssertDebug(!IsUpdating());
        ClearEpoch();
        m_world.m_started = true;
        ++m_epoch;

        const bool fixed = fixedDeltaSeconds > 0;
        if (!fixed)
        {
            for (WorldService* service : m_world.m_services)
                service->Update(m_world.m_owner);
        }

        m_world.CommitImpl(false);

        m_diagnostics = {};
        m_conflicts.clear();
        m_validated = false;
        m_compiled = false;
        m_executed = false;

        m_state = State::kCollecting;
        for (auto* system : m_systems)
        {
            const auto* time = m_world.m_owner.FindService<WorldTimeService>();
            const float deltaSeconds = fixed ? fixedDeltaSeconds : (time ? time->GetDeltaSeconds() : 0);
            const float unpausedDeltaSeconds = fixed ? fixedDeltaSeconds : (time ? time->GetUnpausedDeltaSeconds() : 0);
            EntityUpdateContext context{ m_world.m_owner, system, m_epoch, deltaSeconds, unpausedDeltaSeconds };
            if (fixed)
                system->FixedUpdate(context);
            else
                system->Update(context);
        }

        m_state = State::kRecording;
    }


    void* EntityScheduler::AllocateEpoch(const size_t size, const size_t alignment)
    {
        void* storage = m_epochArena.allocate(size, alignment);
        if (!storage)
        {
            storage = Memory::DefaultAllocate(size, alignment);
            m_largeCaptures.push_back(storage);
        }

        return storage;
    }


    void* EntityScheduler::AllocateTraversal(const size_t size, const size_t alignment)
    {
        FE_AssertDebug(IsUpdating() && !IsExecuting() && !m_compiled);
        return AllocateEpoch(size, alignment);
    }


    Rc<WaitGroup> EntityScheduler::RecordTraversal(EntityUpdateContext& context, const TraversalDesc& desc)
    {
        FE_AssertDebug(desc.m_callable && desc.m_destroy);
        FE_AssertDebug(&context.m_world == &m_world.m_owner && context.m_epoch == m_epoch && IsUpdating());
        FE_AssertDebug(!IsExecuting() && !m_compiled);
        FE_Assert(desc.m_filter == QueryFilter::kAll || desc.m_cursor, "Addition filters require a persistent change cursor");

        const auto accesses = AllocateEpochArray<QueryAccess>(desc.m_accesses.size());
        std::copy(desc.m_accesses.begin(), desc.m_accesses.end(), accesses.begin());

        Traversal& traversal = m_traversals.emplace_back();
        traversal.m_phase = desc.m_phase;
        traversal.m_accesses = accesses;
        traversal.m_callable = desc.m_callable;
        traversal.m_invoke = desc.m_invoke;
        traversal.m_destroy = desc.m_destroy;
        traversal.m_completion = WaitGroup::Create(1);
        traversal.m_prerequisites.assign(desc.m_prerequisites.begin(), desc.m_prerequisites.end());
        traversal.m_system = context.m_system;
        traversal.m_cursor = desc.m_cursor;
        traversal.m_policy = desc.m_policy;
        traversal.m_filter = desc.m_filter;
        traversal.m_cascade = desc.m_cascade;
        traversal.m_hasWrites = std::any_of(accesses.begin(), accesses.end(), [](const QueryAccess& access) {
            return access.m_write && !access.m_parent && !access.m_excluded;
        });

        return traversal.m_completion;
    }


    Rc<WaitGroup> EntityScheduler::RecordStage(const StageDesc& desc)
    {
        EntityUpdateContext context{ m_world.m_owner, nullptr, m_epoch };
        const auto completion =
            RecordTraversal(context, { desc.m_phase, {}, desc.m_callable, nullptr, desc.m_destroy, desc.m_prerequisites });
        m_traversals.back().m_stageInvoke = desc.m_invoke;
        return completion;
    }


    bool EntityScheduler::AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite)
    {
        FE_AssertDebug(IsUpdating() && !m_compiled);

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
        FE_AssertDebug(IsUpdating() && !m_compiled && !IsExecuting());

        for (const auto& scheduled : m_phases)
        {
            if (scheduled.m_phase == phase)
            {
                FE_Assert(false, "Phase scheduled twice in one epoch");
                return false;
            }
        }

        ScheduledPhase& scheduled = m_phases.emplace_back();
        scheduled.m_phase = phase;
        scheduled.m_prerequisites.assign(prerequisites.begin(), prerequisites.end());
        return true;
    }


    bool EntityScheduler::ValidateSchedule()
    {
        FE_PROFILER_ZONE();
        return m_validated || CompileSchedule(true);
    }


    bool EntityScheduler::CompileSchedule(const bool validate)
    {
        FE_PROFILER_ZONE();
        FE_AssertDebug(IsUpdating() && !IsExecuting() && !IsCollecting());

        m_conflicts.clear();
        m_diagnostics.m_conflictEdges = 0;
        m_diagnostics.m_systemEdges = 0;
        m_diagnostics.m_stageEdges = 0;

        if (!ResolvePhases(validate) || !ValidateTraversals(validate))
            return false;

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<Edge> edges(&temp);
        if (!ResolvePrerequisites(validate, temp, edges))
            return false;

        TempVector order(&temp);
        if (!OrderTraversals(validate, temp, edges, order))
            return false;

        // Distribute the global order into phases; phases run in application order.
        festd::pmr::vector<uint32_t> phaseCounts(m_phases.size(), 0u, &temp);
        for (const auto& traversal : m_traversals)
            ++phaseCounts[traversal.m_phaseIndex];

        for (uint32_t phase = 0; phase < m_phases.size(); ++phase)
        {
            m_phases[phase].m_traversals = AllocateEpochArray<uint32_t>(phaseCounts[phase]);
            phaseCounts[phase] = 0;
        }

        for (const uint32_t index : order)
        {
            const uint32_t phase = m_traversals[index].m_phaseIndex;
            m_phases[phase].m_traversals[phaseCounts[phase]++] = index;
        }

        AddExclusionEdges(edges);
        StoreDependencies(edges);

        m_compiled = true;
        m_validated = validate;
        return true;
    }


    bool EntityScheduler::ResolvePhases(const bool validate)
    {
        for (auto& traversal : m_traversals)
        {
            traversal.m_phaseIndex = kInvalidIndex;
            for (uint32_t phase = 0; phase < m_phases.size(); ++phase)
            {
                if (m_phases[phase].m_phase == traversal.m_phase)
                    traversal.m_phaseIndex = phase;
            }

            if (traversal.m_phaseIndex == kInvalidIndex)
                return Reject(validate, "A submitted traversal phase was omitted");
        }

        return true;
    }


    bool EntityScheduler::ValidateTraversals(const bool validate) const
    {
        if (!validate)
            return true;

        for (uint32_t index = 0; index < m_traversals.size(); ++index)
        {
            const auto& traversal = m_traversals[index];
            if (!traversal.m_invoke && !traversal.m_stageInvoke)
                return Reject(validate, "Traversal requires an invocation adapter");

            const bool parallelQuery = !traversal.m_cascade && traversal.m_policy == ExecutionPolicy::kParallel;
            for (const auto& access : traversal.m_accesses)
            {
                if (!access.m_type.IsValid() || !m_world.m_components.Find(access.m_type))
                    return Reject(validate, "Query references an unregistered component type");

                if (!parallelQuery || !access.m_parent)
                    continue;

                const bool aliasesWrite =
                    std::any_of(traversal.m_accesses.begin(), traversal.m_accesses.end(), [&](const QueryAccess& other) {
                        return !other.m_parent && other.m_write && other.m_type == access.m_type;
                    });
                if (aliasesWrite)
                    return Reject(validate, "Parallel parent reads alias self writes; use a cascade query");
            }

            if (!traversal.m_cursor)
                continue;

            for (uint32_t previous = 0; previous < index; ++previous)
            {
                if (m_traversals[previous].m_cursor == traversal.m_cursor)
                    return Reject(validate, "A change cursor may be consumed only once per epoch");
            }
        }

        return true;
    }


    bool EntityScheduler::ResolvePrerequisites(const bool validate, std::pmr::memory_resource& temp,
                                               festd::pmr::vector<Edge>& edges)
    {
        festd::pmr::unordered_dense_map<const WaitGroup*, uint32_t> producers(&temp);
        producers.reserve(m_traversals.size());
        for (uint32_t index = 0; index < m_traversals.size(); ++index)
            producers.emplace(m_traversals[index].m_completion.Get(), index);

        const auto findProducer = [&producers](const Rc<WaitGroup>& group) {
            const auto found = producers.find(group.Get());
            return found == producers.end() ? kInvalidIndex : found->second;
        };

        for (uint32_t index = 0; index < m_traversals.size(); ++index)
        {
            auto& traversal = m_traversals[index];

            uint32_t externalCount = 0;
            for (const auto& group : traversal.m_prerequisites)
            {
                if (!group)
                    return Reject(validate, "Null traversal prerequisite");

                const uint32_t producer = findProducer(group);
                if (producer == kInvalidIndex)
                {
                    ++externalCount;
                    continue;
                }

                if (m_traversals[producer].m_phaseIndex > traversal.m_phaseIndex)
                    return Reject(validate, "Traversal depends on a later phase");

                edges.push_back({ producer, index });
            }

            // External groups stay alive through m_prerequisites until the epoch ends.
            const auto external = AllocateEpochArray<WaitGroup*>(externalCount);
            externalCount = 0;
            for (const auto& group : traversal.m_prerequisites)
            {
                const auto stored = external.first(externalCount);
                const bool duplicate = festd::find(stored, group.Get()) != stored.end();
                if (!duplicate && findProducer(group) == kInvalidIndex)
                    external[externalCount++] = group.Get();
            }

            traversal.m_externalPrerequisites = external.first(externalCount);
        }

        for (uint32_t phase = 0; phase < m_phases.size(); ++phase)
        {
            for (const auto& group : m_phases[phase].m_prerequisites)
            {
                if (!group)
                    return Reject(validate, "Null phase prerequisite");

                const uint32_t producer = findProducer(group);
                if (producer != kInvalidIndex && m_traversals[producer].m_phaseIndex >= phase)
                    return Reject(validate, "Phase prerequisite depends on itself or a later phase");
            }
        }

        return true;
    }


    bool EntityScheduler::OrderTraversals(const bool validate, std::pmr::memory_resource& temp,
                                          const festd::span<const Edge> edges, TempVector& order) const
    {
        const uint32_t count = m_traversals.size();
        order.reserve(count);

        const bool hasForwardEdge = std::any_of(edges.begin(), edges.end(), [](const Edge& edge) {
            return edge.m_producer >= edge.m_consumer;
        });
        if (!hasForwardEdge)
        {
            for (uint32_t index = 0; index < count; ++index)
                order.push_back(index);

            return true;
        }

        // Kahn's algorithm that keeps recording order wherever explicit dependencies allow it: a traversal released by a
        // later producer moves to a later pass and runs after everything already eligible in the current pass.
        festd::pmr::vector<uint32_t> indegrees(count, 0u, &temp);
        festd::pmr::vector<uint32_t> firstDependent(count + 1, 0u, &temp);
        festd::pmr::vector<uint32_t> dependents(edges.size(), 0u, &temp);
        for (const Edge& edge : edges)
        {
            ++indegrees[edge.m_consumer];
            ++firstDependent[edge.m_producer + 1];
        }

        for (uint32_t index = 0; index < count; ++index)
            firstDependent[index + 1] += firstDependent[index];

        festd::pmr::vector<uint32_t> cursor(firstDependent.begin(), firstDependent.end() - 1, &temp);
        for (const Edge& edge : edges)
            dependents[cursor[edge.m_producer]++] = edge.m_consumer;

        struct ReadyTraversal
        {
            uint32_t m_pass;
            uint32_t m_index;
        };

        const auto later = [](const ReadyTraversal& lhs, const ReadyTraversal& rhs) {
            return lhs.m_pass != rhs.m_pass ? lhs.m_pass > rhs.m_pass : lhs.m_index > rhs.m_index;
        };

        festd::pmr::vector<ReadyTraversal> ready(&temp);
        for (uint32_t index = 0; index < count; ++index)
        {
            if (indegrees[index] == 0)
                ready.push_back({ 0, index });
        }

        eastl::make_heap(ready.begin(), ready.end(), later);
        while (!ready.empty())
        {
            eastl::pop_heap(ready.begin(), ready.end(), later);
            const ReadyTraversal current = ready.back();
            ready.pop_back();
            order.push_back(current.m_index);

            for (uint32_t edge = firstDependent[current.m_index]; edge < firstDependent[current.m_index + 1]; ++edge)
            {
                const uint32_t dependent = dependents[edge];
                if (--indegrees[dependent] != 0)
                    continue;

                ready.push_back({ current.m_pass + (dependent < current.m_index), dependent });
                eastl::push_heap(ready.begin(), ready.end(), later);
            }
        }

        if (order.size() != count)
            return Reject(validate, "Framework traversal dependency cycle");

        return true;
    }


    void EntityScheduler::AddExclusionEdges(festd::pmr::vector<Edge>& edges)
    {
        // Within a phase, later work waits for earlier work it conflicts with; order follows the compiled sequence.
        for (const auto& phase : m_phases)
        {
            for (uint32_t a = 0; a < phase.m_traversals.size(); ++a)
            {
                const uint32_t beforeIndex = phase.m_traversals[a];
                const auto& before = m_traversals[beforeIndex];
                for (uint32_t b = a + 1; b < phase.m_traversals.size(); ++b)
                {
                    const uint32_t afterIndex = phase.m_traversals[b];
                    const auto& after = m_traversals[afterIndex];

                    bool conflict = false;
                    for (const auto& lhs : before.m_accesses)
                    {
                        for (const auto& rhs : after.m_accesses)
                        {
                            const bool sameData = !lhs.m_excluded && !rhs.m_excluded && lhs.m_type == rhs.m_type;
                            if (!sameData || (!lhs.m_write && !rhs.m_write))
                                continue;

                            conflict = true;
                            m_conflicts.push_back({ lhs.m_type, beforeIndex, afterIndex });
                        }
                    }

                    const bool stageExclusion = before.IsStage() || after.IsStage();
                    const bool sequentialSystem =
                        before.m_policy != ExecutionPolicy::kParallel || after.m_policy != ExecutionPolicy::kParallel;
                    const bool systemExclusion = before.m_system && before.m_system == after.m_system && sequentialSystem;
                    if (!conflict && !systemExclusion && !stageExclusion)
                        continue;

                    edges.push_back({ beforeIndex, afterIndex });
                    m_diagnostics.m_conflictEdges += conflict;
                    m_diagnostics.m_systemEdges += systemExclusion;
                    m_diagnostics.m_stageEdges += stageExclusion;
                }
            }
        }
    }


    void EntityScheduler::StoreDependencies(festd::pmr::vector<Edge>& edges)
    {
        std::sort(edges.begin(), edges.end(), [](const Edge& lhs, const Edge& rhs) {
            return lhs.m_consumer != rhs.m_consumer ? lhs.m_consumer < rhs.m_consumer : lhs.m_producer < rhs.m_producer;
        });

        const auto sameEdge = [](const Edge& lhs, const Edge& rhs) {
            return lhs.m_consumer == rhs.m_consumer && lhs.m_producer == rhs.m_producer;
        };
        edges.erase(std::unique(edges.begin(), edges.end(), sameEdge), edges.end());

        const auto producers = AllocateEpochArray<uint32_t>(edges.size());
        for (uint32_t edge = 0; edge < edges.size(); ++edge)
            producers[edge] = edges[edge].m_producer;

        uint32_t first = 0;
        for (uint32_t index = 0; index < m_traversals.size(); ++index)
        {
            uint32_t last = first;
            while (last < edges.size() && edges[last].m_consumer == index)
                ++last;

            m_traversals[index].m_dependencies = producers.subspan(first, last - first);
            first = last;
        }
    }


    bool EntityScheduler::ExecuteSchedule()
    {
        FE_PROFILER_ZONE();

        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_AssertDebug(IsUpdating() && !IsExecuting() && !IsCollecting());

        if (m_executed)
            return true;

        if (!m_compiled)
        {
            const bool compiled = m_validationEnabled ? ValidateSchedule() : CompileSchedule(false);
            if (!compiled)
                return false;
        }

        m_state = State::kExecuting;
        auto restore = festd::defer([&] {
            m_state = State::kRecording;
        });

        for (const auto& phase : m_phases)
        {
            for (const auto& group : phase.m_prerequisites)
                group->Wait();

            for (const uint32_t index : phase.m_traversals)
            {
                const auto& traversal = m_traversals[index];
                auto* job = ::new (AllocateEpoch(sizeof(TraversalJob), alignof(TraversalJob)))
                    TraversalJob(*this, &EntityScheduler::ExecuteTraversal, index);

                job->AddPrerequisites(traversal.m_externalPrerequisites);
                for (const uint32_t dependency : traversal.m_dependencies)
                    job->AddPrerequisite(m_traversals[dependency].m_completion);

                const auto affinity =
                    traversal.RunsOnMainThread() ? Jobs::FiberAffinityMask::kMainThread : Jobs::FiberAffinityMask::kAllForeground;
                job->Dispatch(affinity, nullptr, Jobs::Priority::kNormal);
            }

            {
                std::lock_guard guard(m_world.m_storage.m_versionLock);
                m_diagnostics.m_jobs += phase.m_traversals.size();
            }

            for (const uint32_t index : phase.m_traversals)
                m_traversals[index].m_completion->Wait();
        }

        m_executed = true;
        return true;
    }


    void EntityScheduler::ExecuteTraversal(const uint32_t index)
    {
        auto& traversal = m_traversals[index];

        [[maybe_unused]] const festd::ascii_view name = GetTraversalName(traversal);
        FE_PROFILER_ZONE_TEXT("%.*s", name.length(), name.data());

        if (traversal.IsStage())
        {
            // A structural stage may commit queued edits, but cannot record work into the compiled schedule.
            m_state = State::kStructuralStage;
            traversal.m_stageInvoke(traversal.m_callable);
            m_state = State::kExecuting;
            traversal.m_completion->Signal();
            return;
        }

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<PreparedChunk> chunks(&temp);
        PrepareChunks(traversal, temp, chunks);

        TraversalWork work;
        if (traversal.m_cascade)
            ExecuteCascade(traversal, temp, chunks, work);
        else
            ExecuteRows(traversal, temp, chunks, work);

        PublishChanges(traversal, chunks, work);
        traversal.m_completion->Signal();
    }


    void EntityScheduler::PrepareChunks(const Traversal& traversal, std::pmr::memory_resource& temp,
                                        festd::pmr::vector<PreparedChunk>& chunks)
    {
        FE_PROFILER_ZONE();

        const auto& storage = m_world.m_storage;
        const uint32_t accessCount = traversal.m_accesses.size();

        // Layout matching does not access versions or mutable component values.
        auto* columns =
            static_cast<uint32_t*>(temp.allocate(sizeof(uint32_t) * storage.m_chunks.size() * accessCount, alignof(uint32_t)));
        chunks.reserve(storage.m_chunks.size());
        for (auto* chunk : storage.m_chunks)
        {
            uint32_t* chunkColumns = columns + chunks.size() * accessCount;
            bool matches = true;
            for (uint32_t i = 0; i < accessCount; ++i)
            {
                const auto& access = traversal.m_accesses[i];
                const uint32_t column = chunk->m_archetype.Find(access.m_type);
                chunkColumns[i] = column;
                if (access.m_parent)
                    continue;

                if (access.m_excluded)
                    matches &= column == kInvalidIndex;
                else if (!access.m_optional)
                    matches &= column != kInvalidIndex;
            }

            if (matches)
                chunks.push_back({ chunk, chunkColumns });
        }

        if (!traversal.m_cursor || traversal.m_filter != QueryFilter::kAll)
            return;

        std::lock_guard guard(m_world.m_storage.m_versionLock);
        const auto& cursor = *traversal.m_cursor;
        const bool invalidated =
            cursor.m_structureRevision != storage.m_structureRevision || cursor.m_versionEra != storage.m_changeEra;

        // Parent terms may invalidate matching children even when the parent's own archetype is excluded.
        bool parentChanged = false;
        for (const auto& access : traversal.m_accesses)
        {
            if (!access.m_parent)
                continue;

            for (const auto* chunk : storage.m_chunks)
            {
                const uint32_t column = chunk->m_archetype.Find(access.m_type);
                parentChanged |= column != kInvalidIndex && chunk->m_versions[column] > cursor.m_version;
            }
        }

        const auto changed = [&](const PreparedChunk& entry) {
            if (invalidated || parentChanged)
                return true;

            for (uint32_t i = 0; i < accessCount; ++i)
            {
                const auto& access = traversal.m_accesses[i];
                const uint32_t column = entry.m_columns[i];
                const bool tracked = !access.m_parent && !access.m_excluded && column != kInvalidIndex;
                if (tracked && entry.m_chunk->m_versions[column] > cursor.m_version)
                    return true;
            }

            return false;
        };

        if (traversal.m_cascade)
        {
            // Reprocess all matching descendants, including through filtered ancestors.
            const bool hierarchyChanged = cursor.m_hierarchyRevision != storage.m_hierarchyRevision;
            if (!hierarchyChanged && std::none_of(chunks.begin(), chunks.end(), changed))
                chunks.clear();

            return;
        }

        chunks.erase(std::remove_if(chunks.begin(),
                                    chunks.end(),
                                    [&](const PreparedChunk& entry) {
                                        return !changed(entry);
                                    }),
                     chunks.end());
    }


    bool EntityScheduler::InvokeEntity(const Traversal& traversal, Entity& entity, const PreparedChunk& chunk,
                                       const festd::span<void*> values, bool& changed)
    {
        if (!entity.m_active)
            return false;

        const ChangeCursor* cursor = traversal.m_cursor;
        if (traversal.m_filter == QueryFilter::kNewEntities && entity.m_addedVersion <= cursor->m_additionVersion)
            return false;

        bool hasAddedComponent = false;
        for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
        {
            const auto& access = traversal.m_accesses[i];
            if (access.m_excluded)
                continue;

            Entity* source = access.m_parent ? entity.m_parent.Get() : &entity;
            uint32_t column = chunk.m_columns[i];
            if (access.m_parent && source && source->m_chunk)
                column = source->m_chunk->m_archetype.Find(access.m_type);

            const bool available = source && source->m_active && column != kInvalidIndex
                && (source->m_chunk->Stage(source->m_row, column) & ComponentStage::kActive) != ComponentStage::kNone;
            if (!available && !access.m_optional)
                return false;

            values[i] = available ? source->m_chunk->Get(source->m_row, column) : nullptr;
            if (available && !access.m_parent && traversal.m_filter == QueryFilter::kAddedComponents)
                hasAddedComponent |= source->m_chunk->AddedVersion(source->m_row, column) > cursor->m_additionVersion;
        }

        if (traversal.m_filter == QueryFilter::kAddedComponents && !hasAddedComponent)
            return false;

        auto& fiber = Threading::FiberRuntimeInfo::Get();
        CallbackContext callback{ &m_world.m_owner, &traversal, &entity };
        void* previous = fiber.m_executionContext;
        fiber.m_executionContext = &callback;

        auto restore = festd::defer([&] {
            fiber.m_executionContext = previous;
        });

        const bool wrote = traversal.m_invoke(traversal.m_callable, entity, values.data());
        changed |= traversal.m_hasWrites && wrote;
        return true;
    }


    void EntityScheduler::ExecuteRows(const Traversal& traversal, std::pmr::memory_resource& temp,
                                      const festd::span<PreparedChunk> chunks, TraversalWork& work)
    {
        struct RowRange
        {
            PreparedChunk* m_chunk;
            uint32_t m_first;
            uint32_t m_end;
        };

        // Parallel traversals split chunks into fixed row ranges; other policies visit each chunk as one range.
        const bool parallel = traversal.m_policy == ExecutionPolicy::kParallel;
        festd::pmr::vector<RowRange> ranges(&temp);
        for (auto& chunk : chunks)
        {
            const uint32_t rowCount = chunk.m_chunk->m_count;
            const uint32_t grain = parallel ? kRowsPerBatch : Math::Max(1u, rowCount);
            for (uint32_t first = 0; first < rowCount; first += grain)
                ranges.push_back({ &chunk, first, Math::Min(rowCount, first + grain) });
        }

        [[maybe_unused]] const festd::ascii_view name = GetTraversalName(traversal);
        auto visit = [&](const uint32_t batch) {
            const RowRange& range = ranges[batch];
            FE_PROFILER_ZONE_NAMED("QueryRows");
            ZoneTextF("%.*s | rows %u..%u", name.length(), name.data(), range.m_first, range.m_end);

            festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
            uint32_t count = 0;
            bool changed = false;
            for (uint32_t row = range.m_first; row < range.m_end; ++row)
                count += InvokeEntity(traversal, *range.m_chunk->m_chunk->m_entities[row], *range.m_chunk, values, changed);

            if (changed)
                std::atomic_ref(range.m_chunk->m_changed).store(1, std::memory_order_relaxed);

            work.m_callbacks.fetch_add(count, std::memory_order_relaxed);
        };

        work.m_jobs += RunBatches(parallel, ranges.size(), temp, visit);
    }


    void EntityScheduler::ExecuteCascade(const Traversal& traversal, std::pmr::memory_resource& temp,
                                         const festd::span<PreparedChunk> chunks, TraversalWork& work)
    {
        if (chunks.empty())
            return;

        PrepareCascadeOrder();

        // Visiting in level order completes every parent before its children; only parallel work splits a level.
        festd::pmr::unordered_dense_map<const ArchetypeChunk*, PreparedChunk*> matched(&temp);
        matched.reserve(chunks.size());
        for (auto& chunk : chunks)
            matched.emplace(chunk.m_chunk, &chunk);

        const bool parallel = traversal.m_policy == ExecutionPolicy::kParallel;
        [[maybe_unused]] const festd::ascii_view name = GetTraversalName(traversal);
        for (uint32_t level = 0; level + 1 < m_cascadeLevels.size(); ++level)
        {
            const uint32_t levelBegin = m_cascadeLevels[level];
            const uint32_t levelEnd = m_cascadeLevels[level + 1];
            const uint32_t grain = parallel ? kEntitiesPerTreeBatch : levelEnd - levelBegin;
            const uint32_t batchCount = Math::CeilDivide(levelEnd - levelBegin, grain);

            auto visit = [&](const uint32_t batch) {
                const uint32_t first = levelBegin + batch * grain;
                const uint32_t end = Math::Min(levelEnd, first + grain);
                FE_PROFILER_ZONE_NAMED("CascadeRows");
                ZoneTextF("%.*s | level %u | entities %u..%u", name.length(), name.data(), level, first, end);

                festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
                uint32_t count = 0;
                for (uint32_t i = first; i < end; ++i)
                {
                    Entity& entity = *m_cascadeOrder[i];
                    const auto found = matched.find(entity.m_chunk);
                    if (found == matched.end())
                        continue;

                    bool changed = false;
                    count += InvokeEntity(traversal, entity, *found->second, values, changed);
                    if (changed)
                        std::atomic_ref(found->second->m_changed).store(1, std::memory_order_relaxed);
                }

                work.m_callbacks.fetch_add(count, std::memory_order_relaxed);
            };

            const uint32_t jobs = RunBatches(parallel, batchCount, temp, visit);
            work.m_jobs += jobs;
            work.m_treeBatches += jobs;
        }
    }


    void EntityScheduler::PrepareCascadeOrder()
    {
        FE_PROFILER_ZONE();

        std::lock_guard guard(m_cascadeOrderLock);
        const auto& storage = m_world.m_storage;
        const bool current = m_cascadeHierarchyRevision == storage.m_hierarchyRevision
            && m_cascadeStructureRevision == storage.m_structureRevision;
        if (current)
            return;

        m_cascadeOrder.clear();
        m_cascadeLevels.clear();
        m_cascadeLevels.push_back(0);
        for (const auto& slot : storage.m_slots)
        {
            if (slot.m_entity && !slot.m_entity->m_parent)
                m_cascadeOrder.push_back(slot.m_entity);
        }

        uint32_t first = 0;
        uint32_t end = m_cascadeOrder.size();
        while (first < end)
        {
            m_cascadeLevels.push_back(end);
            for (uint32_t i = first; i < end; ++i)
            {
                for (Entity* child = m_cascadeOrder[i]->m_firstChild; child; child = child->m_nextSibling)
                    m_cascadeOrder.push_back(child);
            }

            first = end;
            end = m_cascadeOrder.size();
        }

        m_cascadeHierarchyRevision = storage.m_hierarchyRevision;
        m_cascadeStructureRevision = storage.m_structureRevision;
    }


    void EntityScheduler::PublishChanges(const Traversal& traversal, const festd::span<const PreparedChunk> chunks,
                                         const TraversalWork& work)
    {
        std::lock_guard guard(m_world.m_storage.m_versionLock);
        auto& storage = m_world.m_storage;

        // Only columns this traversal writes, in chunks where a callback reported a write, receive the new version.
        const bool changedWrites =
            traversal.m_hasWrites && std::any_of(chunks.begin(), chunks.end(), [](const PreparedChunk& entry) {
                return entry.m_changed != 0;
            });
        const uint64_t publishedVersion = changedWrites ? m_world.NextChangeVersion() : storage.m_changeVersion;
        for (const auto& entry : chunks)
        {
            if (!changedWrites || entry.m_changed == 0)
                continue;

            for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
            {
                const auto& access = traversal.m_accesses[i];
                const uint32_t column = entry.m_columns[i];
                if (access.m_write && !access.m_parent && !access.m_excluded && column != kInvalidIndex)
                    entry.m_chunk->m_versions[column] = publishedVersion;
            }
        }

        if (traversal.m_cursor)
        {
            auto& cursor = *traversal.m_cursor;
            cursor.m_version = publishedVersion;
            cursor.m_versionEra = storage.m_changeEra;
            cursor.m_hierarchyRevision = storage.m_hierarchyRevision;
            cursor.m_structureRevision = storage.m_structureRevision;
            cursor.m_additionVersion = storage.m_additionVersion;
        }

        m_diagnostics.m_chunks += chunks.size();
        m_diagnostics.m_jobs += work.m_jobs;
        m_diagnostics.m_treeBatches += work.m_treeBatches;
        m_diagnostics.m_callbacks += work.m_callbacks.load();
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
        FE_AssertDebug(Threading::IsMainThread() && IsUpdating() && !IsExecuting() && !IsCollecting());
        auto cleanup = festd::defer([&] {
            ClearEpoch();
            m_state = State::kIdle;
        });

        const bool completed = m_traversals.empty() || m_executed;
        if (IsValidationEnabled())
            FE_Assert(completed, "Incomplete update schedule");
        else
            FE_AssertDebug(completed, "Incomplete update schedule");

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
        FE_AssertDebug(!IsExecuting() && !IsCollecting());
        ClearEpoch();
        for (uint32_t i = m_systems.size(); i > 0; --i)
            m_systems[i - 1]->Shutdown(m_world.m_owner);

        m_systems.clear();
    }
} // namespace FE::Framework
