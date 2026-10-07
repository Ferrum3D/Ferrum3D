#pragma once
#include <Framework/Entities/EntityResidencySet.h>

namespace FE::Framework
{
    // Runtime ownership/residency group, never a simulation boundary. World owns its lifetime.
    struct EntityRegistry final
    {
        [[nodiscard]] EntityWorld& GetWorld() const
        {
            return *m_world;
        }

        [[nodiscard]] uint64_t GetID() const
        {
            return m_id;
        }

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
