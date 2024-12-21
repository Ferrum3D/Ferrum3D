#pragma once
#include <FeCore/EventBus/EventBus.h>

namespace FE
{
    struct FrameEventArgs final
    {
        uint32_t m_frameIndex;
        float m_deltaTime;
    };


    struct FrameEvents
    {
        virtual void OnFrameStart(const FrameEventArgs&) {}
        virtual void OnUpdate(const FrameEventArgs&) {}
        virtual void OnLateUpdate(const FrameEventArgs&) {}
        virtual void OnFrameEnd(const FrameEventArgs&) {}
    };


    using FrameEventBus = EventBus<FrameEvents>;
} // namespace FE
