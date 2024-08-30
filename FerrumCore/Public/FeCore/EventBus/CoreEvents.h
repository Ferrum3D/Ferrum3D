#pragma once
#include <FeCore/EventBus/EventBus.h>

namespace FE
{
    struct FrameEventArgs final
    {
        uint32_t FrameIndex;
        float DeltaTime;
    };


    struct FrameEvents
    {
        inline virtual void OnFrameStart(const FrameEventArgs&) {}
        inline virtual void OnUpdate(const FrameEventArgs&) {}
        inline virtual void OnLateUpdate(const FrameEventArgs&) {}
        inline virtual void OnFrameEnd(const FrameEventArgs&) {}
    };


    using FrameEventBus = EventBus<FrameEvents>;
} // namespace FE
