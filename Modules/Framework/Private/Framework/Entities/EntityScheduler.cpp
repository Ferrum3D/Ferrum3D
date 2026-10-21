#include <Core/CLI/CommandLine.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldInternal.h>
#include <algorithm>

namespace FE::Framework
{
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
        if (it != m_systems.end())
        {
            system.Shutdown(m_world.m_owner);
            m_systems.erase(it);
        }
    }


    void EntityScheduler::BeginUpdate(const float fixedDeltaSeconds)
    {
        FE_PROFILER_ZONE();

        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_AssertDebug(!IsUpdating());
        ClearEpoch();
        m_world.m_started = true;
        ++m_epoch;
        if (fixedDeltaSeconds == 0)
        {
            for (WorldService* service : m_world.m_services)
                service->Update(m_world.m_owner);
        }

        m_world.CommitImpl(false);

        m_state = State::kUpdating;
        m_diagnostics = {};
        m_conflicts.clear();
        m_validated = false;
        m_compiled = false;
        m_executed = false;
        m_state = State::kCollecting;

        auto restore = festd::defer([&] {
            m_state = State::kUpdating;
        });
        for (auto* system : m_systems)
        {
            EntityUpdateContext context{ m_world.m_owner, system, m_epoch, fixedDeltaSeconds };
            if (fixedDeltaSeconds > 0)
                system->FixedUpdate(context);
            else
                system->Update(context);
        }
    }


    void* EntityScheduler::AllocateTraversal(const size_t size, const size_t alignment)
    {
        FE_AssertDebug(IsUpdating() && !IsExecuting() && !m_compiled);
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
        FE_AssertDebug(desc.m_callable && desc.m_destroy);
        FE_AssertDebug(&context.m_world == &m_world.m_owner && context.m_epoch == m_epoch && IsUpdating());
        FE_AssertDebug(!IsExecuting() && !m_compiled);
        FE_Assert(desc.m_filter == QueryFilter::kAll || desc.m_cursor, "Addition filters require a persistent change cursor");

        Traversal traversal;
        traversal.m_phase = desc.m_phase;
        traversal.m_policy = desc.m_policy;
        traversal.m_cascade = desc.m_cascade;
        traversal.m_cursor = desc.m_cursor;
        traversal.m_filter = desc.m_filter;
        traversal.m_accesses.assign(desc.m_accesses.begin(), desc.m_accesses.end());
        traversal.m_callable = desc.m_callable;
        traversal.m_invoke = desc.m_invoke;
        traversal.m_hasWrites = eastl::any_of(desc.m_accesses.begin(), desc.m_accesses.end(), [](const QueryAccess& access) {
            return access.m_write && !access.m_parent && !access.m_excluded;
        });
        traversal.m_destroy = desc.m_destroy;
        traversal.m_system = context.m_system;
        traversal.m_completion = WaitGroup::Create(1);
        traversal.m_prerequisites.assign(desc.m_prerequisites.begin(), desc.m_prerequisites.end());

        Rc<WaitGroup> result = traversal.m_completion;

        m_traversals.push_back(std::move(traversal));
        return result;
    }


    Rc<WaitGroup> EntityScheduler::RecordStage(const StageDesc& desc)
    {
        EntityUpdateContext context{ m_world.m_owner, nullptr, m_epoch };
        TraversalDesc traversal{ desc.m_phase, {}, desc.m_callable, nullptr, desc.m_destroy, desc.m_prerequisites };
        auto completion = RecordTraversal(context, traversal);
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

        ScheduledPhase scheduled{ phase };
        scheduled.m_prerequisites.assign(prerequisites.begin(), prerequisites.end());
        m_phases.push_back(std::move(scheduled));
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
        for (auto& phase : m_phases)
            phase.m_traversals.clear();

        auto fail = [validate](const char* message) {
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
        };
        auto phaseIndex = [&](const Phase phase) {
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
                if (m_traversals[i].m_completion == group)
                    return i;
            }

            return kInvalidIndex;
        };
        auto addDependency = [](Traversal& traversal, const uint32_t producer) {
            if (festd::find(traversal.m_dependencies, producer) == traversal.m_dependencies.end())
                traversal.m_dependencies.push_back(producer);
        };

        festd::inline_vector<uint32_t, 32> phases;
        bool needsSort = false;
        for (uint32_t index = 0; index < m_traversals.size(); ++index)
        {
            auto& traversal = m_traversals[index];
            traversal.m_dependencies.clear();
            traversal.m_externalPrerequisites.clear();
            const uint32_t phase = phaseIndex(traversal.m_phase);
            if (phase == kInvalidIndex)
                return fail("A submitted traversal phase was omitted");

            phases.push_back(phase);
            if (validate)
            {
                if (!traversal.m_invoke && !traversal.m_stageInvoke)
                    return fail("Traversal requires an invocation adapter");

                for (const auto& access : traversal.m_accesses)
                {
                    if (!access.m_type.IsValid() || !m_world.m_components.Find(access.m_type))
                        return fail("Query references an unregistered component type");

                    if (!traversal.m_cascade && access.m_parent && traversal.m_policy == ExecutionPolicy::kParallel)
                    {
                        for (const auto& other : traversal.m_accesses)
                        {
                            if (!other.m_parent && other.m_write && other.m_type == access.m_type)
                                return fail("Parallel parent reads alias self writes; use a cascade query");
                        }
                    }
                }

                if (traversal.m_cursor)
                {
                    for (uint32_t previous = 0; previous < index; ++previous)
                    {
                        if (m_traversals[previous].m_cursor == traversal.m_cursor)
                            return fail("A change cursor may be consumed only once per epoch");
                    }
                }
            }

            for (const auto& group : traversal.m_prerequisites)
            {
                if (!group)
                    return fail("Null traversal prerequisite");

                const uint32_t producer = producerIndex(group);
                if (producer == kInvalidIndex)
                {
                    if (festd::find(traversal.m_externalPrerequisites, group) == traversal.m_externalPrerequisites.end())
                        traversal.m_externalPrerequisites.push_back(group);

                    continue;
                }

                if (phaseIndex(m_traversals[producer].m_phase) > phase)
                    return fail("Traversal depends on a later phase");

                addDependency(traversal, producer);
                needsSort |= producer >= index;
            }
        }

        for (uint32_t i = 0; i < m_phases.size(); ++i)
        {
            for (const auto& group : m_phases[i].m_prerequisites)
            {
                if (!group)
                    return fail("Null phase prerequisite");

                const uint32_t producer = producerIndex(group);
                if (producer != kInvalidIndex && phases[producer] >= i)
                    return fail("Phase prerequisite depends on itself or a later phase");
            }
        }

        festd::inline_vector<uint32_t, 32> order;
        if (needsSort)
        {
            struct ReadyTraversal
            {
                uint32_t m_pass;
                uint32_t m_index;
            };

            festd::inline_vector<uint32_t, 32> indegrees(m_traversals.size(), 0);
            festd::vector<festd::inline_vector<uint32_t>> dependents(m_traversals.size());
            festd::inline_vector<ReadyTraversal, 32> ready;
            auto later = [](const ReadyTraversal& lhs, const ReadyTraversal& rhs) {
                return lhs.m_pass != rhs.m_pass ? lhs.m_pass > rhs.m_pass : lhs.m_index > rhs.m_index;
            };
            for (uint32_t i = 0; i < m_traversals.size(); ++i)
            {
                indegrees[i] = m_traversals[i].m_dependencies.size();
                for (const uint32_t producer : m_traversals[i].m_dependencies)
                    dependents[producer].push_back(i);

                if (indegrees[i] == 0)
                    ready.push_back({ 0, i });
            }

            // Keep recording order where explicit dependencies allow it.
            eastl::make_heap(ready.begin(), ready.end(), later);
            while (!ready.empty())
            {
                eastl::pop_heap(ready.begin(), ready.end(), later);
                const ReadyTraversal current = ready.back();
                ready.pop_back();
                order.push_back(current.m_index);
                for (const uint32_t dependent : dependents[current.m_index])
                {
                    if (--indegrees[dependent] == 0)
                    {
                        ready.push_back({ current.m_pass + (dependent < current.m_index), dependent });
                        eastl::push_heap(ready.begin(), ready.end(), later);
                    }
                }
            }

            if (order.size() != m_traversals.size())
                return fail("Framework traversal dependency cycle");
        }
        else
        {
            for (uint32_t i = 0; i < m_traversals.size(); ++i)
                order.push_back(i);
        }

        for (const uint32_t index : order)
            m_phases[phases[index]].m_traversals.push_back(index);

        for (const auto& phase : m_phases)
        {
            for (uint32_t a = 0; a < phase.m_traversals.size(); ++a)
            {
                const uint32_t beforeIndex = phase.m_traversals[a];
                const auto& before = m_traversals[beforeIndex];
                for (uint32_t b = a + 1; b < phase.m_traversals.size(); ++b)
                {
                    const uint32_t afterIndex = phase.m_traversals[b];
                    auto& after = m_traversals[afterIndex];
                    const bool stageExclusion = before.m_stageInvoke || after.m_stageInvoke;
                    bool conflict = false;
                    for (const auto& lhs : before.m_accesses)
                    {
                        for (const auto& rhs : after.m_accesses)
                        {
                            if (!lhs.m_excluded && !rhs.m_excluded && lhs.m_type == rhs.m_type && (lhs.m_write || rhs.m_write))
                            {
                                conflict = true;
                                m_conflicts.push_back({ lhs.m_type, beforeIndex, afterIndex });
                            }
                        }
                    }

                    const bool systemExclusion = before.m_system && before.m_system == after.m_system
                        && (before.m_policy != ExecutionPolicy::kParallel || after.m_policy != ExecutionPolicy::kParallel);
                    if (conflict || systemExclusion || stageExclusion)
                    {
                        addDependency(after, beforeIndex);
                        m_diagnostics.m_conflictEdges += conflict;
                        m_diagnostics.m_systemEdges += systemExclusion;
                        m_diagnostics.m_stageEdges += stageExclusion;
                    }
                }
            }
        }

        m_compiled = true;
        m_validated = validate;
        return true;
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
            m_state = State::kUpdating;
        });

        for (const auto& phase : m_phases)
        {
            for (const auto& group : phase.m_prerequisites)
                group->Wait();

            for (const uint32_t i : phase.m_traversals)
            {
                auto& traversal = m_traversals[i];
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

        m_executed = true;
        return true;
    }


    void EntityScheduler::PrepareTraversal(const Traversal& traversal, festd::vector<PreparedChunk>& chunks)
    {
        FE_PROFILER_ZONE();

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

        if (!traversal.m_cursor || traversal.m_filter != QueryFilter::kAll)
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
            if (!hierarchyChanged && !eastl::any_of(chunks.begin(), chunks.end(), changed))
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
                                       festd::span<void*> values, bool& changed)
    {
        if (!entity.m_active)
            return false;

        if (traversal.m_filter == QueryFilter::kNewEntities && entity.m_addedVersion <= traversal.m_cursor->m_additionVersion)
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

            void* value = source && source->m_active && column != kInvalidIndex
                    && (source->m_chunk->Stage(source->m_row, column) & ComponentStage::kActive) != ComponentStage::kNone
                ? source->m_chunk->Get(source->m_row, column)
                : nullptr;

            if (!value && !access.m_optional)
                return false;

            values[i] = value;
            if (traversal.m_filter == QueryFilter::kAddedComponents && value && !access.m_parent)
            {
                hasAddedComponent |= source->m_chunk->AddedVersion(source->m_row, column) > traversal.m_cursor->m_additionVersion;
            }
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


    void EntityScheduler::PrepareCascadeOrder()
    {
        FE_PROFILER_ZONE();

        std::lock_guard guard(m_cascadeOrderLock);
        const auto& storage = m_world.m_storage;
        if (m_cascadeHierarchyRevision == storage.m_hierarchyRevision
            && m_cascadeStructureRevision == storage.m_structureRevision)
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


    void EntityScheduler::ExecuteRowBatches(const Traversal& traversal, const festd::span<PreparedChunk> chunks,
                                            TraversalWork& work)
    {
        constexpr uint32_t kRowsPerBatch = 32;
        const bool parallel = traversal.m_policy == ExecutionPolicy::kParallel;
        const festd::ascii_view systemName = traversal.m_system ? traversal.m_system->RTTI_GetDynamicType().m_name : "Query";
        uint32_t batchCount = 0;
        for (const auto& chunk : chunks)
            batchCount += Math::CeilDivide(chunk.m_chunk->m_count, kRowsPerBatch);

        const Rc<WaitGroup> completion = parallel && batchCount > 1 ? WaitGroup::Create() : nullptr;
        for (auto& chunk : chunks)
        {
            const uint32_t rowCount = chunk.m_chunk->m_count;
            const uint32_t grain = parallel ? kRowsPerBatch : Math::Max(1u, rowCount);
            for (uint32_t first = 0; first < rowCount; first += grain)
            {
                const uint32_t end = Math::Min(rowCount, first + grain);
                auto visitBatch = [&, entry = &chunk, first, end] {
                    FE_PROFILER_ZONE_NAMED("QueryRows");
                    ZoneTextF("%.*s | rows %u..%u", systemName.length(), systemName.data(), first, end);

                    festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
                    uint32_t count = 0;
                    bool changed = false;
                    for (uint32_t row = first; row < end; ++row)
                    {
                        if (InvokeEntity(traversal, *entry->m_chunk->m_entities[row], *entry, values, changed))
                            ++count;
                    }

                    if (changed)
                        std::atomic_ref(entry->m_changed).store(1, std::memory_order_relaxed);

                    work.m_callbacks.fetch_add(count, std::memory_order_relaxed);
                };
                if (completion)
                {
                    completion->Add(1);
                    ++work.m_jobs;
                    Jobs::DispatchForeground(std::move(visitBatch), completion.Get());
                }
                else
                {
                    visitBatch();
                }
            }
        }

        if (completion)
        {
            FE_PROFILER_ZONE_NAMED("JoinQueryJobs");
            completion->Signal();
            completion->Wait();
        }
    }


    void EntityScheduler::ExecuteCascadeBatches(const Traversal& traversal, const festd::span<PreparedChunk> chunks,
                                                TraversalWork& work)
    {
        if (chunks.empty())
            return;

        PrepareCascadeOrder();

        festd::unordered_dense_map<const ArchetypeChunk*, uint32_t> chunkLookup;
        for (uint32_t i = 0; i < chunks.size(); ++i)
            chunkLookup.emplace(chunks[i].m_chunk, i);

        const auto visit = [&](Entity& entity, const festd::span<void*> values) {
            const auto found = chunkLookup.find(entity.m_chunk);
            if (found == chunkLookup.end())
                return 0u;

            PreparedChunk& chunk = chunks[found->second];
            bool changed = false;
            const bool invoked = InvokeEntity(traversal, entity, chunk, values, changed);
            if (changed)
                std::atomic_ref(chunk.m_changed).store(1, std::memory_order_relaxed);

            return uint32_t(invoked);
        };
        if (traversal.m_policy != ExecutionPolicy::kParallel)
        {
            festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
            const uint32_t rootCount = m_cascadeLevels.size() > 1 ? m_cascadeLevels[1] : 0;
            uint32_t count = 0;
            for (uint32_t i = 0; i < rootCount; ++i)
            {
                Entity* root = m_cascadeOrder[i];
                Entity* entity = root;
                while (entity)
                {
                    count += visit(*entity, values);
                    if (entity->m_firstChild)
                    {
                        entity = entity->m_firstChild;
                        continue;
                    }

                    while (entity != root && !entity->m_nextSibling)
                        entity = entity->m_parent;

                    entity = entity == root ? nullptr : entity->m_nextSibling.Get();
                }
            }

            work.m_callbacks += count;
            return;
        }

        constexpr uint32_t kEntitiesPerBatch = 16;
        const festd::ascii_view systemName = traversal.m_system ? traversal.m_system->RTTI_GetDynamicType().m_name : "Query";
        for (uint32_t level = 0; level + 1 < m_cascadeLevels.size(); ++level)
        {
            const uint32_t levelEnd = m_cascadeLevels[level + 1];
            const uint32_t levelBegin = m_cascadeLevels[level];
            const bool subdivide = levelEnd - levelBegin > kEntitiesPerBatch;
            const Rc<WaitGroup> completion = subdivide ? WaitGroup::Create() : nullptr;
            for (uint32_t first = levelBegin; first < levelEnd; first += kEntitiesPerBatch)
            {
                const uint32_t end = Math::Min(levelEnd, first + kEntitiesPerBatch);
                auto visitBatch = [&, first, end, level] {
                    FE_PROFILER_ZONE_NAMED("CascadeRows");
                    ZoneTextF("%.*s | level %u | entities %u..%u", systemName.length(), systemName.data(), level, first, end);

                    festd::inline_vector<void*> values(traversal.m_accesses.size(), nullptr);
                    uint32_t count = 0;
                    for (uint32_t i = first; i < end; ++i)
                        count += visit(*m_cascadeOrder[i], values);

                    work.m_callbacks.fetch_add(count, std::memory_order_relaxed);
                };
                if (completion)
                {
                    completion->Add(1);
                    ++work.m_jobs;
                    ++work.m_treeBatches;
                    Jobs::DispatchForeground(std::move(visitBatch), completion.Get());
                }
                else
                {
                    visitBatch();
                }
            }

            if (completion)
            {
                FE_PROFILER_ZONE_NAMED("JoinCascadeLevel");
                completion->Signal();
                completion->Wait();
            }
        }
    }


    void EntityScheduler::ExecuteTraversal(const uint32_t index)
    {
        auto& traversal = m_traversals[index];

        [[maybe_unused]] festd::ascii_view systemName = "Query";
        if (traversal.m_system)
            systemName = traversal.m_system->RTTI_GetDynamicType().m_name;

        FE_PROFILER_ZONE_TEXT("%.*s", systemName.length(), systemName.data());

        if (traversal.m_stageInvoke)
        {
            // A structural stage may commit queued edits, but cannot record work into the compiled schedule.
            m_state = State::kStructuralStage;
            traversal.m_stageInvoke(traversal.m_callable);
            m_state = State::kExecuting;
            traversal.m_completion->Signal();
            return;
        }

        festd::vector<PreparedChunk> chunks;
        PrepareTraversal(traversal, chunks);

        TraversalWork work;
        if (traversal.m_cascade)
            ExecuteCascadeBatches(traversal, chunks, work);
        else
            ExecuteRowBatches(traversal, chunks, work);

        {
            std::lock_guard guard(m_world.m_storage.m_versionLock);

            const bool writes = traversal.m_hasWrites;
            const bool changedWrites = writes && eastl::any_of(chunks.begin(), chunks.end(), [](const PreparedChunk& entry) {
                                           return entry.m_changed != 0;
                                       });
            const uint64_t publishedVersion = changedWrites ? m_world.NextChangeVersion() : m_world.m_storage.m_changeVersion;
            if (changedWrites)
            {
                for (const auto& entry : chunks)
                {
                    if (entry.m_changed == 0)
                        continue;

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
                traversal.m_cursor->m_additionVersion = m_world.m_storage.m_additionVersion;
            }
            m_diagnostics.m_chunks += chunks.size();
            m_diagnostics.m_jobs += work.m_jobs;
            m_diagnostics.m_treeBatches += work.m_treeBatches;
            m_diagnostics.m_callbacks += work.m_callbacks.load();
        }

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
