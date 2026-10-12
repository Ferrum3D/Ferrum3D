#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Input/WorldInputService.h>
#include <GameFramework/CameraMovementComponent.h>
#include <GameFramework/TransformComponents.h>

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
        void Shutdown(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;

        static bool ApplyMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                  const CameraInput& input, float deltaSeconds);

    private:
        Input::WorldInputService* m_input = nullptr;
    };
} // namespace FE::GameFramework
