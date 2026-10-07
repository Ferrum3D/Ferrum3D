#pragma once
#include <Core/Env/Environment.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/RTTI/Reflection.h>

namespace FE::Framework
{
    struct Entity;
    struct EntityWorld;
    struct EntityRegistry;
    struct Archetype;
    struct ArchetypeChunk;
    struct EntityCommandList;
    struct EntityUpdateContext;
    struct WorldSystem;

    struct EntityID final
    {
        uint64_t m_value = 0;
        [[nodiscard]] uint16_t World() const
        {
            return static_cast<uint16_t>(m_value >> 48);
        }


        [[nodiscard]] uint32_t Slot() const
        {
            return static_cast<uint32_t>((m_value >> 24) & 0xffffff);
        }


        [[nodiscard]] uint32_t Generation() const
        {
            return static_cast<uint32_t>(m_value & 0xffffff);
        }
        bool operator==(const EntityID&) const = default;
        static EntityID Pack(uint16_t world, uint32_t slot, uint32_t generation)
        {
            return { (uint64_t(world) << 48) | (uint64_t(slot) << 24) | generation };
        }
    };


    enum class LifecycleResult : uint8_t
    {
        kSucceeded,
        kPending,
        kFailed
    };


    enum class ResidencyScope : uint8_t
    {
        kEntity,
        kRegistry
    };


    enum class ExecutionPolicy : uint8_t
    {
        kSequential,
        kParallelChunks,
        kParallelHierarchyTrees
    };


    struct Phase final
    {
        uint64_t m_id;
        festd::ascii_view m_name;
        bool operator==(const Phase& other) const
        {
            return m_id == other.m_id;
        }
    };

#define FE_DECLARE_ENTITY_PHASE(name)                                                                                            \
    inline constexpr ::FE::Framework::Phase name                                                                                 \
    {                                                                                                                            \
        ::FE::CompileTimeHash(#name, sizeof(#name) - 1), #name                                                                   \
    }
    namespace Phases
    {
        FE_DECLARE_ENTITY_PHASE(PreUpdate);
        FE_DECLARE_ENTITY_PHASE(Update);
        FE_DECLARE_ENTITY_PHASE(PostUpdate);
    } // namespace Phases


    // Parent terms are declared here; topology-aware execution is introduced with the parallel scheduler.
    template<class T>
    struct Parent
    {
        using Type = T;
    };
} // namespace FE::Framework
