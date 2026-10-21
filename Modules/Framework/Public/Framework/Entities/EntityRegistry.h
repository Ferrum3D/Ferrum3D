#pragma once
#include <Framework/Entities/EntityResidencySet.h>

namespace FE::Framework
{
    //! @brief World-owned ownership and asset residency group, not a simulation boundary.
    struct EntityRegistry final
    {
        [[nodiscard]] EntityWorld& GetWorld() const
        {
            return *m_world;
        }

        //! @brief Return the reserved world-local runtime ID used to reject commands for removed registries.
        [[nodiscard]] uint32_t GetID() const
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
        friend struct EntityLifecycle;
        friend struct EntityMaterializer;

        EntityWorld* m_world;
        const uint32_t m_id;
        bool m_unloading = false;
        const Uuid m_key;
        EntityResidencySet m_residency;

        EntityRegistry(EntityWorld& world, EntityAssetServices& services, const uint32_t id, const Uuid key)
            : m_world(&world)
            , m_id(id)
            , m_key(key)
            , m_residency(services)
        {
        }
    };
} // namespace FE::Framework
