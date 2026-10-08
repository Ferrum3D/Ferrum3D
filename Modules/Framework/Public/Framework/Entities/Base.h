#pragma once
#include <Core/Env/Environment.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/RTTI/Reflection.h>

namespace FE::Framework
{
    //! @brief Stable runtime metadata; component addresses remain valid only until structural commit.
    struct Entity;
    //! @brief Own entities, storage, and a shared schedule across all registries; safe points require the main thread.
    struct EntityWorld;
    //! @brief World-owned ownership and asset residency group, not a simulation boundary.
    struct EntityRegistry;
    struct Archetype;
    struct ArchetypeChunk;
    //! @brief Single-recorder transactional batch; independent conflicting batches are rejected.
    struct EntityCommandList;
    struct EntityUpdateContext;
    struct WorldSystem;

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

        //! @brief Compare all packed world, slot, and generation bits.
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
        //! @brief The operation failed and runtime state must be unwound.
        kFailed = 1 << 5
    };
    FE_ENUM_OPERATORS(ComponentStage);


    //! @brief Result of a nonblocking lifecycle hook; Pending is retried at later safe points.
    enum class LifecycleResult : uint8_t
    {
        //! @brief The operation completed successfully.
        kSucceeded,
        //! @brief The operation is not ready; poll it at a later safe point.
        kPending,
        //! @brief The operation failed and runtime state must be unwound.
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


    //! @brief Select sequential callbacks or query-appropriate parallel work.
    enum class ExecutionPolicy : uint8_t
    {
        //! @brief Process one traversal sequentially; exclude other work from the same system.
        kSequential,
        //! @brief Process chunks, or parent-before-child hierarchy-tree batches for cascades, concurrently.
        kParallel
    };


    //! @brief Stable schedule key plus a borrowed diagnostic name; phases execute in explicit application order.
    struct Phase final
    {
        //! @brief Stable identity key.
        uint64_t m_id;
        //! @brief Display name; not an identity key.
        festd::ascii_view m_name;

        //! @brief Compare identity; phase names are diagnostic and do not participate in equality.
        bool operator==(const Phase& other) const
        {
            //! @brief Stable identity key.
            return m_id == other.m_id;
        }
    };

//! @brief Declare an inline phase key from its identifier, with a static diagnostic name.
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


    //! @brief Read-only query term for the immediate active parent; required terms skip roots.
    // Parent terms read the active immediate parent; cascade queries additionally enforce topological order.
    template<class T>
    struct Parent
    {
        //! @brief Component term resolved on the immediate active parent.
        using Type = T;
    };


    //! @brief Exclude entities whose archetype contains T; contributes no callback argument or component access.
    template<class T>
    struct Without
    {
        //! @brief Component type whose presence excludes an entity.
        using Type = T;
    };
} // namespace FE::Framework
