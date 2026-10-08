#pragma once
#include <Framework/Entities/EntityResidencySet.h>

namespace FE::Framework
{
    // Runtime ownership/residency group, never a simulation boundary. World owns its lifetime.
    //! @brief World-owned ownership and asset residency group, not a simulation boundary.
    struct EntityRegistry final
    {
        //! @brief Return the owning world; registries never define separate simulation boundaries.
        [[nodiscard]] EntityWorld& GetWorld() const
        {
            //! @brief Owning world or its incarnation; borrowed where represented as a reference.
            return *m_world;
        }

        //! @brief Return the world-local incarnation used to reject commands for removed registries.
        [[nodiscard]] uint64_t GetID() const
        {
            //! @brief Stable identity key.
            return m_id;
        }

        //! @brief Inspect shared acquisitions used by entities with registry-scoped residency.
        [[nodiscard]] const EntityResidencySet& GetResidency() const
        {
            return m_residency;
        }

    private:
        friend EntityWorld;
        EntityWorld* m_world;
        const uint64_t m_id;
        EntityResidencySet m_residency;
        bool m_unloading = false;

        EntityRegistry(EntityWorld& world, EntityAssetServices& services, uint64_t id)
            : m_world(&world)
            , m_id(id)
            , m_residency(services)
        {
        }
    };
} // namespace FE::Framework
