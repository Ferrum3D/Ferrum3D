#pragma once
#include <Core/RTTI/Any.h>
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Entities/EntityWorldAsset.h>

namespace FE::Framework
{
    //! @brief Own a world and its reflected systems and services. Services are initialized before systems and survive entity teardown.
    struct EntityWorldInstance final
    {
        //! @brief Create an empty instance with borrowed asset services.
        explicit EntityWorldInstance(EntityAssetServices* assets = nullptr);
        //! @brief Remove all membership, then shut down and destroy the selected systems and services.
        ~EntityWorldInstance();
        EntityWorldInstance(const EntityWorldInstance&) = delete;
        EntityWorldInstance& operator=(const EntityWorldInstance&) = delete;
        //! @brief Construct an empty instance from a definition; invalid system/service types or content leave it empty.
        bool Load(const EntityWorldAsset& definition);
        //! @brief Restore concrete state into an empty instance without respawning source collections.
        bool Restore(const EntityWorldSnapshotAsset& snapshot);
        //! @brief Report initial materialization/restore completion, including failed or canceled loads.
        [[nodiscard]] MaterializationStatus GetStatus() const;
        //! @brief Save concrete state and the selected system/service IDs at a settled main-thread boundary.
        bool Capture(EntityWorldSnapshotAsset& snapshot) const;
        //! @brief Borrow the independently owned runtime world.
        [[nodiscard]] EntityWorld& GetWorld()
        {
            return m_world;
        }

    private:
        void ClearObjects();
        EntityWorld m_world;
        festd::vector<Rtti::Any> m_systems;
        festd::vector<Rtti::Any> m_services;
        festd::vector<MaterializationToken> m_operations;
    };
} // namespace FE::Framework
