#pragma once
#include <Graphics/Core/Fence.h>

namespace FE::Graphics::Core
{
    struct Buffer;
    struct Texture;

    struct GraphicsQueue : public DeviceObject
    {
        FE_RTTI("BFA35DB4-E1AA-4914-87FB-D392B0308B34");

        ~GraphicsQueue() override = default;

        virtual FenceSyncPoint GetCurrentFence() const = 0;

        // Queue a transfer-owned asset for acquisition before the next frame's draw commands.
        virtual void PublishShaderRead(Buffer* buffer) = 0;
        virtual void PublishShaderRead(Texture* texture) = 0;

        //! Reserve the frame timeline; command storage waits are deferred until recording starts.
        virtual void BeginFrame() = 0;
        //! Submit a frame without a presentation image (for suspended/out-of-date viewports).
        virtual void SubmitFrame() = 0;
        virtual FenceSyncPoint CloseFrame() = 0;
        virtual void Drain() = 0;
    };
} // namespace FE::Graphics::Core
