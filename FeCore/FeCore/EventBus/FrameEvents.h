#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    struct FrameEventArgs
    {
        UInt32 FrameIndex;
        Float32 DeltaTime;
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
