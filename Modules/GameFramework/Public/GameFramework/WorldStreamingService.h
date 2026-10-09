#pragma once
#include <Framework/Entities/EntityWorld.h>

namespace FE::GameFramework
{
    //! @brief Main-thread placement streaming controller; registries remain part of one shared world schedule.
    struct WorldStreamingService final : Framework::WorldService
    {
        FE_RTTI("aaa69126-4427-4055-b1c4-04013aae10a4");

        //! @brief Queue a persistent placement load into a registry identified by its saved key.
        void Load(Uuid registry, IO::AssetID placement);
        //! @brief Queue removal of a registry, including cancellation of pending placements.
        void Unload(Uuid registry);
        //! @brief Inspect the latest load for a placement; unavailable assets remain recoverable failures.
        [[nodiscard]] Framework::MaterializationStatus GetStatus(IO::AssetID placement) const;
        //! @brief Attach to one world; the service must outlive its registration.
        void Init(Framework::EntityWorld& world) override;
        //! @brief Discard queued requests and detach from the world.
        void Shutdown(Framework::EntityWorld& world) override;
        //! @brief Apply queued requests before structural commit at the main-thread boundary.
        void Update(Framework::EntityWorld& world) override;

    private:
        struct Request
        {
            Uuid m_registry;
            IO::AssetID m_placement;
        };

        struct Entry
        {
            IO::AssetID m_placement;
            Framework::MaterializationToken m_token;
        };

        Framework::EntityWorld* m_world = nullptr;
        festd::vector<Request> m_requests;
        festd::vector<Entry> m_entries;
    };
} // namespace FE::GameFramework
