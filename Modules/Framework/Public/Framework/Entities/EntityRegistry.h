#pragma once
#include <Framework/Entities/EntityResidencySet.h>

namespace FE::Framework
{
    // Runtime ownership/residency group, never a simulation boundary. World owns its lifetime.
    //! @brief World-owned ownership and asset residency group, not a simulation boundary.
    struct EntityRegistry final
    {
        [[nodiscard]] EntityWorld& GetWorld() const
        {
            return *m_world;
        }

        //! @brief Return the world-local incarnation used to reject commands for removed registries.
        [[nodiscard]] uint64_t GetID() const
        {
            return m_id;
        }

        //! @brief Inspect shared acquisitions used by entities with registry-scoped residency.
        [[nodiscard]] const EntityResidencySet& GetResidency() const
        {
            return m_residency;
        }

        //! @brief Persistent ownership key used by world definitions and snapshots.
        [[nodiscard]] Uuid GetKey() const
        {
            return m_key;
        }

    private:
        friend EntityWorld;
        EntityWorld* m_world;
        const uint64_t m_id;
        const Uuid m_key;
        EntityResidencySet m_residency;
        bool m_unloading = false;

        EntityRegistry(EntityWorld& world, EntityAssetServices& services, uint64_t id, Uuid key)
            : m_world(&world)
            , m_id(id)
            , m_key(key)
            , m_residency(services)
        {
        }
    };
} // namespace FE::Framework
