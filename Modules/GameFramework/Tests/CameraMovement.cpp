#include <GameFramework/CameraMovementSystem.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::GameFramework;

TEST(CameraMovement, MovementUsesTimeAndMouseUsesPixelDelta)
{
    CameraMovementComponent movement;
    movement.m_moveAction = IO::Link<Input::Action>(Uuid::Random());
    movement.m_elevateAction = IO::Link<Input::Action>(Uuid::Random());
    movement.m_lookAction = IO::Link<Input::Action>(Uuid::Random());
    TransformComponent first, second;
    Input::ResolvedAction actions[3];
    actions[0].m_action = movement.m_moveAction.GetAssetID();
    actions[0].m_type = Input::ActionType::kVector2;
    actions[0].m_value = { 1, 1 };
    actions[0].m_active = true;
    actions[1].m_action = movement.m_elevateAction.GetAssetID();
    actions[1].m_type = Input::ActionType::kScalar;
    actions[1].m_value = { 1, 0 };
    actions[1].m_active = true;
    CameraMovementSystem::ApplyMovement(movement, first, actions, 0.5f);
    CameraMovementSystem::ApplyMovement(movement, second, actions, 0.25f);
    CameraMovementSystem::ApplyMovement(movement, second, actions, 0.25f);
    EXPECT_TRUE(Math::CmpEqual(first.m_local.Translation(), Vector3{ 2, 2, 2 }));
    EXPECT_TRUE(Math::CmpEqual(first.m_local.Translation(), second.m_local.Translation()));

    actions[0].m_active = false;
    actions[1].m_active = false;
    actions[2].m_action = movement.m_lookAction.GetAssetID();
    actions[2].m_type = Input::ActionType::kVector2;
    actions[2].m_value = { 20, -10 };
    actions[2].m_active = true;
    first.m_local = second.m_local = Transform::Identity();
    CameraMovementSystem::ApplyMovement(movement, first, actions, 0.01f);
    CameraMovementSystem::ApplyMovement(movement, second, actions, 0.1f);
    EXPECT_TRUE(Math::CmpEqual(Math::Rotate(Vector3{ 0, 0, 1 }, first.m_local.Rotation()),
                               Math::Rotate(Vector3{ 0, 0, 1 }, second.m_local.Rotation())));
}


TEST(CameraMovement, MouseRotatesHorizontallyAndVerticallyAndStopsWhenInactive)
{
    CameraMovementComponent movement;
    movement.m_lookAction = IO::Link<Input::Action>(Uuid::Random());
    movement.m_mouseSensitivity = 1;
    Input::ResolvedAction look;
    look.m_action = movement.m_lookAction.GetAssetID();
    look.m_type = Input::ActionType::kVector2;
    look.m_active = true;
    TransformComponent transform;
    look.m_value = { Constants::kPI / 2, 0 };
    CameraMovementSystem::ApplyMovement(movement, transform, { &look, 1 }, 0.1f);
    EXPECT_TRUE(Math::CmpEqual(Math::Rotate(Vector3::WorldForward(), transform.m_local.Rotation()), Vector3{ 1, 0, 0 }, 1e-5f));

    transform.m_local = Transform::Identity();
    look.m_value = { 0, Constants::kPI / 2 };
    CameraMovementSystem::ApplyMovement(movement, transform, { &look, 1 }, 0.1f);
    EXPECT_TRUE(Math::CmpEqual(Math::Rotate(Vector3::WorldForward(), transform.m_local.Rotation()), Vector3{ 0, -1, 0 }, 1e-5f));

    const Quaternion rotation = transform.m_local.Rotation();
    look.m_active = false;
    CameraMovementSystem::ApplyMovement(movement, transform, { &look, 1 }, 0.1f);
    EXPECT_EQ(transform.m_local.Rotation(), rotation);
}
