#pragma once
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    namespace Internal
    {
        struct Traversal
        {
            Phase m_phase;
            festd::vector<QueryAccess> m_accesses;
            void* m_callable = nullptr;
            TraversalInvoke m_invoke = nullptr;
            CallableDestroy m_destroy = nullptr;
            Rc<WaitGroup> m_completion;
            festd::vector<Rc<WaitGroup>> m_prerequisites;
            WorldSystem* m_system = nullptr;
            StageInvoke m_stageInvoke = nullptr;
            ChangeCursor* m_cursor = nullptr;
            festd::vector<uint32_t> m_dependencies;

            struct Mapping
            {
                const Archetype* m_archetype;
                festd::vector<uint32_t> m_columns;
            };

            festd::vector<Mapping> m_mappings;
            ExecutionPolicy m_policy = ExecutionPolicy::kSequential;
            bool m_executed = false;
            bool m_cascade = false;
        };


        struct CallbackContext
        {
            const EntityWorld* m_world;
            const Traversal* m_traversal;
            const Entity* m_entity;
        };


        struct ScheduledPhase
        {
            Phase m_phase;
            festd::vector<Rc<WaitGroup>> m_prerequisites;
        };
    } // namespace Internal


    using Internal::ScheduledPhase;
    using Internal::Traversal;

    // Embedded epoch owner: plan validation and dispatch share one state lifetime without a separate allocation.
    struct EntityScheduler final
    {
        explicit EntityScheduler(EntityWorld::Impl& world)
            : m_world(world)
        {
        }

        void AddSystem(WorldSystem& system);
        void RemoveSystem(WorldSystem& system);
        void BeginUpdate();
        bool EndUpdate();
        void ClearEpoch();
        void Shutdown();
        void* AllocateTraversal(size_t size, size_t alignment);
        Rc<WaitGroup> RecordTraversal(EntityUpdateContext& context, const TraversalDesc& desc);
        Rc<WaitGroup> RecordStage(const StageDesc& desc);
        bool AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite);
        bool SchedulePhase(Phase phase, festd::span<const Rc<WaitGroup>> prerequisites);
        bool ValidateSchedule();
        bool ExecuteSchedule();
        ScheduleDiagnostics GetDiagnostics() const;
        festd::span<const ScheduleConflict> GetConflicts() const;

        uint64_t m_epoch = 0;
        bool m_collecting = false;
        bool m_updating = false;
        bool m_executing = false;

    private:
        void PrepareTraversal(Traversal& traversal, festd::vector<ArchetypeChunk*>& chunks);
        bool InvokeEntity(Traversal& traversal, Entity& entity, const festd::vector<ArchetypeChunk*>& chunks);
        void ExecuteTraversal(uint32_t index);

        EntityWorld::Impl& m_world;
        Memory::LinearAllocator m_epochArena;
        festd::vector<void*> m_largeCaptures;
        festd::vector<WorldSystem*> m_systems;
        festd::vector<Traversal> m_traversals;
        festd::vector<ScheduledPhase> m_phases;
        festd::vector<ScheduleConflict> m_conflicts;
        ScheduleDiagnostics m_diagnostics;
        bool m_validated = false;
    };
} // namespace FE::Framework
