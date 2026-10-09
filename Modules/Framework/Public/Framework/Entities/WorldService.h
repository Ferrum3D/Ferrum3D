#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    //! @brief World integration object with main-thread lifetime and updates outside traversal collection.
    struct WorldService
    {
        FE_RTTI("aaa69126-4427-4055-b1c4-04013aae09fd");

        //! @brief Destroy after detaching from the world.
        virtual ~WorldService() = default;
        //! @brief Initialize before components or dependent systems are created.
        virtual void Init(EntityWorld&) {}
        //! @brief Release integration after world entities have been cleared.
        virtual void Shutdown(EntityWorld&) {}
        //! @brief Apply queued requests before structural commit, outside the update schedule.
        virtual void Update(EntityWorld&) {}

    private:
        friend struct EntityWorld;
    };
} // namespace FE::Framework
