#pragma once
#include <Core/Memory/Memory.h>
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    struct EntityWorldSystem
    {
        virtual ~EntityWorldSystem() = default;

        virtual void Init() {}
        virtual void Shutdown() {}

        virtual void Destroy() = 0;

        virtual void Update(const EntityUpdateContext& context) = 0;

        virtual void RegisterArchetype([[maybe_unused]] const Archetype* archetype) {}
        virtual void UnregisterArchetype([[maybe_unused]] const Archetype* archetype) {}

    protected:
        EntityWorldSystem() = default;
    };
} // namespace FE::Framework
