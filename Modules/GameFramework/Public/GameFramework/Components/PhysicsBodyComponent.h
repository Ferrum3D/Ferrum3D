#pragma once
#include <Core/Math/Transform.h>
#include <Framework/Entities/EntityComponentRegistry.h>
#include <GameFramework/Assets/PhysicsShapeAsset.h>

namespace FE::GameFramework
{
    enum class PhysicsMotionType : uint32_t
    {
        kStatic,
        kDynamic,
        kKinematic,
    };


    //! @brief Rigid body at a transform root. Collision scale is baked at activation; replace the body to rebuild it.
    struct PhysicsBodyComponent final
    {
        festd::vector<PhysicsBodyShape> m_shapes;
        PhysicsMotionType m_motionType = PhysicsMotionType::kDynamic;
        float m_mass = 1.0f;
        float m_friction = 0.5f;
        float m_restitution = 0.0f;
        Vector3 m_linearVelocity = Vector3::kZero;
        Vector3 m_angularVelocity = Vector3::kZero;

        FE_SKIP_SERIALIZING uint32_t m_body = UINT32_MAX;
        FE_SKIP_SERIALIZING Transform m_lastTransform = Transform::Identity();

        PhysicsBodyComponent() = default;
        PhysicsBodyComponent(const PhysicsBodyComponent& other);
        PhysicsBodyComponent(PhysicsBodyComponent&&) noexcept = default;

        Framework::LifecycleResult Activate(Framework::ComponentContext& context);
        void Deactivate(Framework::ComponentContext& context);

        FE_RTTI_Reflect("A48CD6FB-337B-44F2-8CF4-91FCA06FCB86");
        FE_RTTI_Serialize();
    };
} // namespace FE::GameFramework
