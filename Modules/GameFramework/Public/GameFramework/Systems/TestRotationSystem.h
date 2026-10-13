#pragma once
#include <Framework/Entities/EntityWorld.h>

namespace FE::GameFramework
{
    struct TestRotationSystem final : Framework::WorldSystem
    {
        FE_RTTI("3596AA48-D7BD-4E79-8535-657A7B44E37A");

        void Update(Framework::EntityUpdateContext& context) override;
    };
} // namespace FE::GameFramework
