#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <HAL/Common/Viewport.h>

namespace FE::Graphics::HAL
{
    struct WindowDesc
    {
        uint32_t Width{};
        uint32_t Height{};

        StringSlice Title{};
    };


    class IWindow : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IWindow, "2E09CD62-42A4-4E0D-BC2C-B11E849FBEAF");

        ~IWindow() override = default;

        virtual ResultCode Init(const WindowDesc& desc) = 0;

        virtual void PollEvents() = 0;
        virtual bool CloseRequested() = 0;
        virtual void* GetNativeHandle() = 0;
        virtual Viewport CreateViewport() = 0;
        virtual Scissor CreateScissor() = 0;
    };
} // namespace FE::Graphics::HAL
