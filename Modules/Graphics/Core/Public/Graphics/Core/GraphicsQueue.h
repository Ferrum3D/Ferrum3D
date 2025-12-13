#pragma once
#include <Graphics/Core/Fence.h>

namespace FE::Graphics::Core
{
    struct GraphicsQueue : public DeviceObject
    {
        FE_RTTI("BFA35DB4-E1AA-4914-87FB-D392B0308B34");

        ~GraphicsQueue() override = default;

        virtual void BeginFrame() = 0;
        virtual FenceSyncPoint CloseFrame() = 0;
        virtual void Drain() = 0;
    };
} // namespace FE::Graphics::Core
