#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Fiber.h>
#include <Framework/Entities/EntityWorld.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    namespace Internal
    {
        uint64_t NextCommandListID();
        constexpr uint8_t kDiscovered = 1;
        constexpr uint8_t kLoading = 2;
        constexpr uint8_t kLoaded = 4;
        constexpr uint8_t kInitialized = 8;
        constexpr uint8_t kActive = 16;
        constexpr uint8_t kFailed = 32;

        enum class CommandKind : uint8_t
        {
            kCreate,
            kDestroy,
            kRename,
            kParent,
            kActive,
            kUnload,
            kUnloadRegistry,
            kComponent,
            kRemove
        };


        struct Command
        {
            CommandKind m_kind;
            EntityTarget m_target;
            EntityTarget m_parent;
            EntityRegistry* m_registry = nullptr;
            uint64_t m_registryId = 0;
            Env::Name m_name;
            Uuid m_uuid = Uuid::kNull;
            const EntityComponentInfo* m_component = nullptr;
            Rtti::TypeID m_type = Rtti::TypeID::kNull;
            void* m_payload = nullptr;
            bool m_active = true;
            ResidencyScope m_residency = ResidencyScope::kEntity;
        };


        struct AssetContribution
        {
            Rtti::TypeID m_component;
            IO::AssetID m_asset;
            Rtti::TypeID m_expectedType;
            uint64_t m_transition = 0;
        };


        struct Traversal
        {
            Phase m_phase;
            ExecutionPolicy m_policy = ExecutionPolicy::kSequential;
            festd::vector<QueryAccess> m_accesses;
            void* m_callable = nullptr;
            void (*m_invoke)(void*, Entity&, void**) = nullptr;
            void (*m_destroy)(void*) = nullptr;
            Rc<WaitGroup> m_completion;
            festd::vector<Rc<WaitGroup>> m_prerequisites;
            WorldSystem* m_system = nullptr;
            void (*m_stageInvoke)(void*) = nullptr;
            bool m_executed = false;
            bool m_cascade = false;
            ChangeCursor* m_cursor = nullptr;
            festd::vector<uint32_t> m_dependencies;
            struct Mapping
            {
                const Archetype* m_archetype;
                festd::vector<uint32_t> m_columns;
                bool m_matches = true;
            };
            festd::vector<Mapping> m_mappings;
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

    using namespace Internal;

    struct Entity::Runtime
    {
        struct Replacement
        {
            const EntityComponentInfo* m_info;
            void* m_data;
            uint64_t m_transition;
            uint8_t m_stage = 0;
        };
        ResidencyScope m_residencyScope = ResidencyScope::kEntity;
        EntityResidencySet* m_residency = nullptr;
        festd::inline_vector<Replacement> m_replacements;
        festd::inline_vector<AssetContribution> m_assets;
        uint32_t m_unreadyChildren = 0;
        bool m_prepared = false;
    };


    struct EntityCommandList::Impl
    {
        EntityWorld* m_world;
        uint64_t m_id = Internal::NextCommandListID();
        uint32_t m_created = 0;
        uint64_t m_eligibleEpoch = 0;
        Memory::LinearAllocator m_arena;
        festd::vector<Command> m_commands;
        festd::vector<void*> m_largePayloads;
        explicit Impl(EntityWorld& world)
            : m_world(&world)
        {
        }


        ~Impl()
        {
            for (const auto& command : m_commands)
            {
                if (command.m_payload)
                    command.m_component->m_type->m_destructor(command.m_payload);
            }
            for (void* payload : m_largePayloads)
                Memory::DefaultFree(payload);
        }
    };


    struct EntityWorld::Impl
    {
        struct Slot
        {
            Entity* m_entity = nullptr;
            uint32_t m_generation = 1;
        };
        Memory::Pool<Entity> m_entities{ "Entity/WorldPool" };
        EntityComponentRegistry m_components;
        EntityAssetServices* m_assets;
        void* m_services;
        uint16_t m_token;
        festd::vector<Slot> m_slots;
        festd::vector<uint32_t> m_freeSlots;
        festd::unordered_dense_map<Uuid, Entity*> m_uuidLookup;
        festd::vector<EntityRegistry*> m_registries;
        festd::vector<Archetype*> m_archetypes;
        festd::vector<ArchetypeChunk*> m_chunks;
        Threading::SpinLock m_commandLock;
        festd::vector<EntityCommandList::Impl*> m_commands;
        festd::vector<WorldSystem*> m_systems;
        Memory::LinearAllocator m_epochArena;
        festd::vector<void*> m_largeCaptures;
        festd::vector<Traversal> m_traversals;
        festd::vector<ScheduledPhase> m_phases;
        festd::ascii_view m_error;
        uint64_t m_nextRegistryId = 1;
        uint64_t m_epoch = 0;
        uint64_t m_hierarchyRevision = 0;
        uint64_t m_structureRevision = 0;
        bool m_started = false;
        bool m_collecting = false;
        bool m_updating = false;
        bool m_executing = false;
        bool m_validated = false;
        bool m_scheduleFailed = false;
        Threading::SpinLock m_versionLock;
        uint64_t m_changeVersion = 1;
        ScheduleDiagnostics m_diagnostics;
        festd::vector<ScheduleConflict> m_conflicts;
        Rtti::TypeID m_loadingComponent = Rtti::TypeID::kNull;
        uint64_t m_loadingTransition = 0;
        uint64_t m_nextTransition = 1;

        void ClearEpoch()
        {
            for (auto& traversal : m_traversals)
            {
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
    };

} // namespace FE::Framework
