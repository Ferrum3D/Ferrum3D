#pragma once
#include <FeCore/Components/LocalToWorldComponent.h>
#include <FeCore/Components/PositionComponent.h>
#include <FeCore/ECS/ComponentSystem.h>
#include <FeCore/ECS/EntityQuery.h>

namespace FE::ECS
{
    class TransformSystem final : public ComponentSystem
    {
        Rc<EntityQuery> m_Query;

    public:
        FE_CLASS_RTTI(TransformSystem, "0E8270F1-6D58-4D56-9519-A3A306E93FA1");

        ~TransformSystem() override = default;

        void OnCreate() override;
        void OnUpdate(const FrameEventArgs& args) override;
    };
} // namespace FE::ECS
