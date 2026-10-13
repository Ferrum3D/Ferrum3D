#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Phases.h>

namespace FE::GameFramework
{
    //! @brief Register cameras and extract changed camera settings/world transforms after Transformation.
    struct CameraSystem final : Framework::WorldSystem
    {
        FE_RTTI("E2E58CE8-7202-4B65-91C7-B13145B938A2");

        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;

    private:
        Framework::ChangeCursor m_changes;
    };
} // namespace FE::GameFramework
