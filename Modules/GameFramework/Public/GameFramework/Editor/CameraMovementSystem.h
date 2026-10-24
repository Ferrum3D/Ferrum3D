#pragma once
#include <GameFramework/Components/CameraMovementComponent.h>
#include <GameFramework/Systems/CameraMovementSystem.h>

namespace FE::Editor
{
    struct CameraMovementComponent final
    {
        FE_RTTI_Reflect("EC3E5A9D-9D92-40BF-B106-99DBEBD8AAEF");
        FE_RTTI_Serialize();

        GameFramework::CameraMovementComponent m_movement;
        Input::ActionReference<bool> m_speedUpAction;
        float m_speedUpFactor = 4;
    };


    struct CameraMovementSystem final : Framework::WorldSystem
    {
        FE_RTTI("CC4720A6-5762-4924-ACDC-C0759E3AFFAF");

        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;

        Vector2UInt m_viewportSize{ 1, 1 };
        bool m_inputEnabled = false;
    };
} // namespace FE::Editor
