#pragma once
#include <FeCore/Memory/Object.h>
#include <FeCore/EventBus/FrameEvents.h>

namespace FE::ECS
{
    class ISystem : public IObject
    {
    public:
        FE_CLASS_RTTI(ISystem, "71ADC4BC-A185-47B5-89FA-08F5427158C7");

        virtual void OnCreate() = 0;
        virtual void OnUpdate(const FrameEventArgs& args) = 0;
        virtual void OnDestroy() = 0;
    };
} // namespace FE::ECS
