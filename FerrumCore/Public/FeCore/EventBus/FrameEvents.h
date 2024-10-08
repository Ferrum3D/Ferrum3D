#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    struct FrameEventArgs
    {
        uint32_t FrameIndex;
        float DeltaTime;
    };

    class FrameEvents
    {
    public:
        inline virtual void OnFrameStart(const FrameEventArgs&) {}
        inline virtual void OnUpdate(const FrameEventArgs&) {}
        inline virtual void OnLateUpdate(const FrameEventArgs&) {}
        inline virtual void OnFrameEnd(const FrameEventArgs&) {}
    };
} // namespace FE
