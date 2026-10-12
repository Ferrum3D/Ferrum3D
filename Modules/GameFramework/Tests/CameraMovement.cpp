#include <GameFramework/CameraMovementSystem.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::GameFramework;

TEST(CameraMovement, MovementUsesTimeAndLookUsesAngularDisplacement)
{
    CameraMovementComponent movement;
    CameraInput input{ { 1, 1 }, 1, { 0, 0 } };
    TransformComponent first, second;
    EXPECT_TRUE(CameraMovementSystem::ApplyMovement(movement, first, input, 0.5f));
    EXPECT_TRUE(CameraMovementSystem::ApplyMovement(movement, second, input, 0.25f));
    EXPECT_TRUE(CameraMovementSystem::ApplyMovement(movement, second, input, 0.25f));
    EXPECT_TRUE(Math::CmpEqual(first.m_local.Translation(), Vector3{ 2, 2, 2 }));
    EXPECT_TRUE(Math::CmpEqual(first.m_local.Translation(), second.m_local.Translation()));

    input = { { 0, 0 }, 0, { 0.06f, -0.03f } };
    first.m_local = second.m_local = Transform::Identity();
    CameraMovementSystem::ApplyMovement(movement, first, input, 0.01f);
    CameraMovementSystem::ApplyMovement(movement, second, input, 0.1f);
    EXPECT_TRUE(Math::CmpEqual(Math::Rotate(Vector3::WorldForward(), first.m_local.Rotation()),
                               Math::Rotate(Vector3::WorldForward(), second.m_local.Rotation())));
}


TEST(CameraMovement, BothRotationAxesAndIdleMovement)
{
    CameraMovementComponent movement;
    CameraInput input;
    input.m_look = { Constants::kPI / 2, 0 };
    TransformComponent transform;
    CameraMovementSystem::ApplyMovement(movement, transform, input, 0.1f);
    EXPECT_TRUE(Math::CmpEqual(Math::Rotate(Vector3::WorldForward(), transform.m_local.Rotation()), Vector3{ 1, 0, 0 }, 1e-5f));

    transform.m_local = Transform::Identity();
    input.m_look = { 0, Constants::kPI / 2 };
    CameraMovementSystem::ApplyMovement(movement, transform, input, 0.1f);
    EXPECT_TRUE(Math::CmpEqual(Math::Rotate(Vector3::WorldForward(), transform.m_local.Rotation()), Vector3{ 0, -1, 0 }, 1e-5f));

    const Quaternion rotation = transform.m_local.Rotation();
    EXPECT_FALSE(CameraMovementSystem::ApplyMovement(movement, transform, {}, 0.1f));
    EXPECT_EQ(transform.m_local.Rotation(), rotation);
}
