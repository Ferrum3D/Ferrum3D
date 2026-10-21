#pragma once
#include <Framework/Entities/WorldService.h>
#include <GameFramework/Components/PhysicsBodyComponent.h>

namespace FE::GameFramework
{
    //! @brief Own rigid bodies and Jolt integration for one world. All API calls require a world safe point or physics stage.
    struct WorldPhysicsService final : Framework::WorldService
    {
        FE_RTTI("B37E58C0-D8E1-41F8-968A-5E5F9F03EE86");

        WorldPhysicsService();
        ~WorldPhysicsService() override;
        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;

        bool CreateBody(const PhysicsBodyComponent& definition, const Transform& transform, Vector3 scale, uint32_t& body);
        void DestroyBody(uint32_t body);
        void SetTransform(uint32_t body, const Transform& transform, PhysicsMotionType motion, float deltaSeconds);
        [[nodiscard]] Transform GetTransform(uint32_t body, float scale) const;
        void GetVelocity(uint32_t body, Vector3& linear, Vector3& angular) const;
        [[nodiscard]] uint32_t GetBodyCount() const;
        void Step(float deltaSeconds);

    private:
        struct Impl;
        Impl* m_impl = nullptr;
    };
} // namespace FE::GameFramework
