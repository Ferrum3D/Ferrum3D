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
                                               const festd::span<const Rc<WaitGroup>> prerequisites, const ExecutionPolicy policy)
    {
        FE_Assert(&context.m_world == this && context.m_epoch == m_impl->m_epoch && m_impl->m_updating);
        FE_Assert(!m_impl->m_executing && !m_impl->m_validated);
        Traversal traversal;
        traversal.m_phase = phase;
        traversal.m_policy = policy;
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
            if (traversal.m_policy != ExecutionPolicy::kSequential)
                return Fail("Parallel execution policies require the Stage 6 scheduler");
            const uint32_t phase = phaseIndex(traversal.m_phase);
            if (phase == kInvalidIndex)
                return Fail("A submitted traversal phase was omitted");
            for (const auto& access : traversal.m_accesses)
            {
                if (access.m_parent)
                    return Fail("Parent traversal execution requires the Stage 6 topology scheduler");
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
                ++count;
                progress = true;
            }
            if (!progress)
                return Fail("Framework traversal dependency cycle");
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
            m_impl->m_currentTraversal = nullptr;
        });
        for (const auto& phase : m_impl->m_phases)
        {
            for (const auto& group : phase.m_prerequisites)
                group->Wait();
            uint32_t remaining = 0;
            for (const auto& traversal : m_impl->m_traversals)
            {
                if (traversal.m_phase == phase.m_phase && !traversal.m_executed)
                    ++remaining;
            }
            while (remaining)
            {
                bool progress = false;
                for (auto& traversal : m_impl->m_traversals)
                {
                    if (!(traversal.m_phase == phase.m_phase) || traversal.m_executed)
                        continue;
                    bool ready = true;
                    for (const auto& group : traversal.m_prerequisites)
                    {
                        for (const auto& producer : m_impl->m_traversals)
                        {
                            if (producer.m_completion.Get() == group.Get() && !producer.m_executed)
                                ready = false;
                        }
                    }
                    if (!ready)
                        continue;
                    for (const auto& group : traversal.m_prerequisites)
                        group->Wait();
                    festd::inline_vector<void*> values;
                    values.resize(traversal.m_accesses.size());
                    m_impl->m_currentTraversal = &traversal;
                    if (traversal.m_stageInvoke)
                    {
                        m_impl->m_currentTraversal = nullptr;
                        m_impl->m_executing = false;
                        traversal.m_stageInvoke(traversal.m_callable);
                        m_impl->m_executing = true;
                    }
                    for (auto* chunk : m_impl->m_chunks)
                    {
                        if (traversal.m_stageInvoke)
                            break;

                        auto mapping =
                            festd::find_if(traversal.m_mappings.begin(), traversal.m_mappings.end(), [&](const auto& entry) {
                                return entry.m_archetype == &chunk->m_archetype;
                            });
                        if (mapping == traversal.m_mappings.end())
                        {
                            Traversal::Mapping entry{ &chunk->m_archetype };
                            for (const auto& access : traversal.m_accesses)
                            {
                                const uint32_t column = chunk->m_archetype.Find(access.m_type);
                                entry.m_columns.push_back(column);
                                if (column == kInvalidIndex && !access.m_optional)
                                    entry.m_matches = false;
                            }

                            traversal.m_mappings.push_back(std::move(entry));
                            mapping = traversal.m_mappings.end() - 1;
                        }
                        if (!mapping->m_matches)
                            continue;
                        for (uint32_t row = 0; row < chunk->m_count; ++row)
                        {
                            Entity& entity = *chunk->m_entities[row];
                            if (!entity.m_active)
                                continue;
                            bool matches = true;
                            for (uint32_t i = 0; i < traversal.m_accesses.size(); ++i)
                            {
                                const uint32_t column = mapping->m_columns[i];
                                values[i] = column != kInvalidIndex && (chunk->Stage(row, column) & kActive)
                                    ? chunk->Get(row, column)
                                    : nullptr;
                                if (!values[i] && !traversal.m_accesses[i].m_optional)
                                    matches = false;
                            }
                            if (matches)
                                traversal.m_invoke(traversal.m_callable, entity, values.data());
                        }
                    }
                    m_impl->m_currentTraversal = nullptr;
                    traversal.m_executed = true;
                    traversal.m_completion->Signal();
                    --remaining;
                    progress = true;
                }
                FE_Assert(progress, "Validated schedule made no progress");
            }
        }
        return true;
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
