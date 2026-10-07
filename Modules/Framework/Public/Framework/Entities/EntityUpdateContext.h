#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    struct EntityUpdateContext final
    {
        EntityWorld& m_world;
        WorldSystem* m_system = nullptr;
        uint64_t m_epoch = 0;
    };


    // Application owns systems through Shutdown. Collection cannot inspect live component data or wait on traversals.
    struct WorldSystem
    {
        virtual ~WorldSystem() = default;
        virtual void Init(EntityWorld&) {}

        virtual void Shutdown(EntityWorld&) {}
        virtual void Update(EntityUpdateContext&) = 0;
    };
} // namespace FE::Framework
