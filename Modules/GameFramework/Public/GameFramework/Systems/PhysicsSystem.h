#pragma once
#include <Framework/Entities/EntityWorld.h>

namespace FE::GameFramework
{
    struct PhysicsSystem final : Framework::WorldSystem
    {
        FE_RTTI("EF92DC49-16FD-4DF7-98DE-AC50D718F2C0");

        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext&) override {}
        void FixedUpdate(Framework::EntityUpdateContext& context) override;
    };
} // namespace FE::GameFramework
