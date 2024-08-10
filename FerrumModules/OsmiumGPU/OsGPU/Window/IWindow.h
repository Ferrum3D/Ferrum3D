#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <OsGPU/Common/Viewport.h>

namespace FE::Osmium
{
    struct WindowDesc
    {
        FE_RTTI_Base(WindowDesc, "A32F5560-2333-4BE1-A661-59EAC29ABFBC");

        uint32_t Width{};
        uint32_t Height{};

        StringSlice Title{};
    };

    class IWindow : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IWindow, "2E09CD62-42A4-4E0D-BC2C-B11E849FBEAF");

        ~IWindow() override = default;

        virtual void PollEvents() = 0;
        virtual bool CloseRequested() = 0;
        virtual void* GetNativeHandle() = 0;
        virtual Viewport CreateViewport() = 0;
        virtual Scissor CreateScissor() = 0;
    };
} // namespace FE::Osmium
