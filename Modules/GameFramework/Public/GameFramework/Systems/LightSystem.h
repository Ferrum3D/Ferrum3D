#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Phases.h>

namespace FE::GameFramework
{
    //! Submit changed light parameters and world transforms to persistent scene slots.
    struct LightSystem final : Framework::WorldSystem
    {
        FE_RTTI("52A039AE-EE95-411F-AD79-D779B4AB2CDF");
        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;

    private:
        Framework::ChangeCursor m_pointChanges;
        Framework::ChangeCursor m_spotChanges;
    };
} // namespace FE::GameFramework
