#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Input/WorldInputService.h>

namespace FE::GameFramework
{
    struct CameraInput final
    {
        Vector2 m_move{ 0, 0 };
        float m_elevate = 0;
        Vector2 m_look{ 0, 0 };
    };


    struct CameraMovementSystem final : public Framework::WorldSystem
    {
        FE_RTTI("9A548BD5-6768-49BC-9FE6-77F418F8C9C5");

        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;
    };
} // namespace FE::GameFramework
