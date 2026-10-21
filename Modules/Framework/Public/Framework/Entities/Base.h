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
    struct WorldService;

    //! @brief Generation-checked runtime identity, valid only within its owning world incarnation.
    struct EntityID final
    {
        //! @brief Packed runtime identity; zero is invalid.
        uint64_t m_value = 0;

        //! @brief Return the world incarnation encoded in this runtime ID.
        [[nodiscard]] uint16_t World() const
        {
            return static_cast<uint16_t>(m_value >> 48);
        }

        //! @brief Return the 24-bit stable world slot index.
        [[nodiscard]] uint32_t Slot() const
        {
            return static_cast<uint32_t>((m_value >> 24) & 0xffffff);
        }

        //! @brief Return the 24-bit slot generation used to reject stale IDs.
        [[nodiscard]] uint32_t Generation() const
        {
            return static_cast<uint32_t>(m_value & 0xffffff);
        }

        bool operator==(const EntityID&) const = default;

        //! @brief Combine a world incarnation, slot, and generation; inputs must fit their reserved bit widths.
        static EntityID Pack(const uint16_t world, const uint32_t slot, const uint32_t generation)
        {
            return { (static_cast<uint64_t>(world) << 48) | (static_cast<uint64_t>(slot) << 24) | generation };
        }
    };


    //! @brief Per-row lifecycle state; flags are carried with values during migration.
    enum class ComponentStage : uint8_t
    {
        //! @brief No lifecycle stages completed.
        kNone = 0,
        //! @brief Serialized hard dependencies have been discovered.
        kDiscovered = 1 << 0,
        //! @brief Loading has started and must be undone during teardown.
        kLoading = 1 << 1,
        //! @brief Loading and required asset readiness completed.
        kLoaded = 1 << 2,
        //! @brief Initialization completed.
        kInitialized = 1 << 3,
        //! @brief Activation completed; group publication may still be pending.
        kActive = 1 << 4,
        kFailed = 1 << 5
    };
    FE_ENUM_OPERATORS(ComponentStage);


    //! @brief Result of a nonblocking lifecycle hook; Pending is retried at later safe points.
    enum class LifecycleResult : uint8_t
    {
        kSucceeded,
        kPending,
        kFailed
    };


    //! @brief Select which transform representation is preserved by a hierarchy edit.
    enum class ReparentMode : uint8_t
    {
        //! @brief Adjust authored local values to preserve the evaluated world transform.
        kPreserveWorld,
        //! @brief Keep authored local values while changing inherited world transform.
        kPreserveLocal
    };


    //! @brief Choose whether asset acquisitions are deduplicated per entity or per registry.
    enum class ResidencyScope : uint8_t
    {
        //! @brief Deduplicate asset acquisitions within one entity.
        kEntity,
        //! @brief Deduplicate asset acquisitions across a registry.
        kRegistry
    };


    //! @brief Select sequential callbacks, main-thread callbacks or query-appropriate parallel work.
    enum class ExecutionPolicy : uint8_t
    {
        //! @brief Process one traversal sequentially; exclude other work from the same system.
        kSequential,
        //! @brief Process row ranges concurrently; cascades join each hierarchy level before processing its children.
        kParallel,
        //! @brief Process one traversal sequentially on the main thread; exclude other work from the same system.
        kMainThread
    };


    //! @brief Schedule key; phases execute in explicit application order at either update frequency.
    struct Phase final
    {
        //! @brief Stable identity key.
        uint64_t m_id;

        bool operator==(const Phase&) const = default;
    };

//! @brief Declare a phase key by hashing its identifier.
#define FE_DECLARE_ENTITY_PHASE(name)                                                                                            \
    inline constexpr ::FE::Framework::Phase name                                                                                 \
    {                                                                                                                            \
        ::FE::CompileTimeHash(#name, sizeof(#name) - 1)                                                                          \
    }

    namespace Phases
    {
        FE_DECLARE_ENTITY_PHASE(PreUpdate);
        FE_DECLARE_ENTITY_PHASE(Update);
        FE_DECLARE_ENTITY_PHASE(PostUpdate);
    } // namespace Phases


    //! @brief Read-only query term for the immediate active parent; required terms skip roots.
    //!
    //! Cascade queries additionally enforce topological order.
    template<class T>
    struct Parent
    {
        using Type = T;
    };


    //! @brief Exclude entities whose archetype contains T; contributes no callback argument or component access.
    template<class T>
    struct Without
    {
        using Type = T;
    };
} // namespace FE::Framework
