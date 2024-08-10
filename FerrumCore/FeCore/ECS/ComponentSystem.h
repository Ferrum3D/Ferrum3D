#pragma once
#include <FeCore/ECS/EntityRegistry.h>
#include <FeCore/ECS/ISystem.h>
#include <FeCore/ECS/World.h>

namespace FE::ECS
{
    class ComponentSystem : public ISystem
    {
        EntityRegistry* m_Registry;

    public:
        FE_RTTI_Class(ComponentSystem, "6E69ACBC-6A89-439E-BF1A-57A80AB59039");

        inline ComponentSystem()
        {
            m_Registry = ServiceLocator<IWorld>::Get()->Registry();
        }

        ~ComponentSystem() override = default;

        void OnCreate() override {}
        void OnUpdate(const FrameEventArgs& /* args */) override {}
        void OnDestroy() override {}

        [[nodiscard]] inline EntityRegistry* Registry() const
        {
            return m_Registry;
        }
    };
} // namespace FE::ECS
