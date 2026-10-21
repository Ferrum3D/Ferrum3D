#pragma once
#include <Core/Threading/FiberMutex.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    namespace Internal
    {
        //! @brief One recorded query traversal or main-thread application stage of the open epoch.
        //! Spans point into the scheduler's epoch arena and are released by ClearEpoch.
        struct Traversal
        {
            Phase m_phase;
            festd::span<const QueryAccess> m_accesses;
            void* m_callable = nullptr;
            //! @brief Query row callback; null for stages.
            TraversalInvoke m_invoke = nullptr;
            //! @brief Stage callback; null for queries.
            StageInvoke m_stageInvoke = nullptr;
            CallableDestroy m_destroy = nullptr;
            Rc<WaitGroup> m_completion;
            //! @brief Recorded prerequisites; also keep external wait groups alive for execution.
            festd::inline_vector<Rc<WaitGroup>, 2> m_prerequisites;
            //! @brief Recording system, or null for application work.
            WorldSystem* m_system = nullptr;
            ChangeCursor* m_cursor = nullptr;

            //! @brief Compiled producer traversal indices, including exclusion edges.
            festd::span<const uint32_t> m_dependencies;
            //! @brief Compiled prerequisites that do not belong to this epoch.
            festd::span<WaitGroup* const> m_externalPrerequisites;
            uint32_t m_phaseIndex = kInvalidIndex;

            ExecutionPolicy m_policy = ExecutionPolicy::kSequential;
            QueryFilter m_filter = QueryFilter::kAll;
            bool m_cascade = false;
            //! @brief Writes a local component, so successful callbacks publish change versions.
            bool m_hasWrites = false;

            [[nodiscard]] bool IsStage() const
            {
                return m_stageInvoke != nullptr;
            }

            [[nodiscard]] bool RunsOnMainThread() const
            {
                return IsStage() || m_policy == ExecutionPolicy::kMainThread;
            }
        };


        //! @brief Fiber-local callback state used to check component lookups against the traversal's declared access.
        struct CallbackContext
        {
            const EntityWorld* m_world;
            const Traversal* m_traversal;
            const Entity* m_entity;
        };


        struct ScheduledPhase
        {
            Phase m_phase;
            festd::inline_vector<Rc<WaitGroup>, 2> m_prerequisites;
            //! @brief Compiled execution order of this phase's traversals.
            festd::span<uint32_t> m_traversals;
        };
    } // namespace Internal


    using Internal::ScheduledPhase;
    using Internal::Traversal;

    //! @brief Record, compile and execute one epoch of traversals.
    //!
    //! An epoch moves through collection (systems record traversals), recording (the application schedules phases),
    //! compilation (dependency and exclusion edges) and execution. Every per-epoch allocation lives in the epoch arena or
    //! in fiber-local temporary memory.
    struct EntityScheduler final
    {
        explicit EntityScheduler(EntityWorld::Impl& world);

        void AddSystem(WorldSystem& system);
        void RemoveSystem(WorldSystem& system);
        void BeginUpdate(float fixedDeltaSeconds = 0);
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

        [[nodiscard]] bool IsCollecting() const
        {
            return m_state == State::kCollecting;
        }


        [[nodiscard]] bool IsUpdating() const
        {
            return m_state != State::kIdle;
        }


        [[nodiscard]] bool IsExecuting() const
        {
            return m_state == State::kExecuting;
        }


        [[nodiscard]] bool IsValidationEnabled() const
        {
#if FE_DEBUG
            return true;
#else
            return m_validationEnabled || m_validated;
#endif
        }

    private:
        enum class State : uint8_t
        {
            kIdle,
            //! @brief Systems are recording traversals and must not touch component data.
            kCollecting,
            //! @brief The epoch is open for application scheduling, compilation or a completed execution.
            kRecording,
            kExecuting,
            //! @brief A main-thread stage is running and may commit structural changes.
            kStructuralStage
        };

        //! @brief A producer-before-consumer edge between traversal indices.
        struct Edge
        {
            uint32_t m_producer;
            uint32_t m_consumer;
        };

        //! @brief A matched chunk and the column of each query access, or kInvalidIndex.
        struct PreparedChunk
        {
            ArchetypeChunk* m_chunk;
            const uint32_t* m_columns;
            mutable uint32_t m_changed = 0;
        };

        struct TraversalWork
        {
            std::atomic<uint32_t> m_callbacks = 0;
            uint32_t m_jobs = 0;
            uint32_t m_treeBatches = 0;
        };

        using TempVector = festd::pmr::vector<uint32_t>;

        // Compilation steps, in order.
        bool CompileSchedule(bool validate);
        bool ResolvePhases(bool validate);
        bool ValidateTraversals(bool validate) const;
        bool ResolvePrerequisites(bool validate, std::pmr::memory_resource& temp, festd::pmr::vector<Edge>& edges);
        bool OrderTraversals(bool validate, std::pmr::memory_resource& temp, festd::span<const Edge> edges,
                             TempVector& order) const;
        void AddExclusionEdges(festd::pmr::vector<Edge>& edges);
        void StoreDependencies(festd::pmr::vector<Edge>& edges);

        void ExecuteTraversal(uint32_t index);
        void PrepareChunks(const Traversal& traversal, std::pmr::memory_resource& temp,
                           festd::pmr::vector<PreparedChunk>& chunks);
        void PrepareCascadeOrder();
        void ExecuteRows(const Traversal& traversal, std::pmr::memory_resource& temp, festd::span<PreparedChunk> chunks,
                         TraversalWork& work);
        void ExecuteCascade(const Traversal& traversal, std::pmr::memory_resource& temp, festd::span<PreparedChunk> chunks,
                            TraversalWork& work);
        bool InvokeEntity(const Traversal& traversal, Entity& entity, const PreparedChunk& chunk, festd::span<void*> values,
                          bool& changed);
        void PublishChanges(const Traversal& traversal, festd::span<const PreparedChunk> chunks, const TraversalWork& work);

        //! @brief Allocate storage released by ClearEpoch; valid at any point of the open epoch on the main thread.
        void* AllocateEpoch(size_t size, size_t alignment);

        template<class T>
        festd::span<T> AllocateEpochArray(const uint32_t count)
        {
            static_assert(std::is_trivially_destructible_v<T>);
            if (count == 0)
                return {};

            return { static_cast<T*>(AllocateEpoch(sizeof(T) * count, alignof(T))), count };
        }

        EntityWorld::Impl& m_world;
        State m_state = State::kIdle;
        Memory::LinearAllocator m_epochArena;
        festd::vector<void*> m_largeCaptures;
        festd::vector<WorldSystem*> m_systems;
        festd::vector<Traversal> m_traversals;
        festd::vector<ScheduledPhase> m_phases;
        festd::vector<ScheduleConflict> m_conflicts;
        ScheduleDiagnostics m_diagnostics;
        Threading::FiberMutex m_cascadeOrderLock;
        festd::vector<Entity*> m_cascadeOrder;
        //! @brief Start offsets of every hierarchy level in m_cascadeOrder, followed by its total size.
        festd::vector<uint32_t> m_cascadeLevels;
        uint64_t m_cascadeHierarchyRevision = UINT64_MAX;
        uint64_t m_cascadeStructureRevision = UINT64_MAX;
        bool m_validated = false;
        bool m_compiled = false;
        bool m_executed = false;
        bool m_validationEnabled;
    };
} // namespace FE::Framework
