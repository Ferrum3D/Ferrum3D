#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    //! @brief Borrowed collection context; valid only while recording work for the current open epoch.
    struct EntityUpdateContext final
    {
        //! @brief Owning world or its incarnation; borrowed where represented as a reference.
        EntityWorld& m_world;
        //! @brief Borrowed collecting system; null for application-submitted work.
        WorldSystem* m_system = nullptr;
        //! @brief Epoch that admitted this update context.
        uint64_t m_epoch = 0;
    };

    // Application owns systems through Shutdown. Collection cannot inspect live component data or wait on traversals.
    //! @brief Application-owned system that records deferred traversals and remains alive until Shutdown.
    struct WorldSystem
    {
        FE_RTTI("9436EA35-32DC-4A82-BEB3-18C9D4ECC76F");

        virtual ~WorldSystem() = default;
        virtual void Init(EntityWorld&) {}

        virtual void Shutdown(EntityWorld&) {}
        //! @brief Record this epoch's traversals; do not inspect live component data or wait during collection.
        virtual void Update(EntityUpdateContext& context) = 0;
    };
} // namespace FE::Framework
