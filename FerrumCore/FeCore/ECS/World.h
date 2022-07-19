#pragma once
#include <FeCore/ECS/EntityRegistry.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <FeCore/Modules/SharedInterface.h>

namespace FE::ECS
{
    class ISystem;

    class IWorld : public IObject
    {
    public:
        FE_CLASS_RTTI(IWorld, "DCA8359C-CDD0-4555-8B87-2D2F5915476F");

        virtual void RegisterSystem(ISystem* system) = 0;
        virtual void UnregisterSystem(ISystem* system) = 0;

        virtual EntityRegistry* Registry() = 0;
    };

    class World final : public SharedInterfaceImplBase<IWorld>, public EventBus<FrameEvents>::Handler
    {
        List<Shared<ISystem>> m_Systems;
        Shared<EntityRegistry> m_Registry;

    public:
        FE_CLASS_RTTI(World, "1937CC6F-4309-4EF4-9AB2-014ABC269FBE");

        World();
        ~World() override = default;

        void RegisterSystem(ISystem* system) override;
        void UnregisterSystem(ISystem* system) override;

        EntityRegistry* Registry() override;

        void OnUpdate(const FrameEventArgs& args) override;
    };
} // namespace FE::ECS
