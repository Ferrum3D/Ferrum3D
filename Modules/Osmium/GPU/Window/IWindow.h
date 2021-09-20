#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <GPU/Common/Viewport.h>

namespace FE::GPU
{
    struct WindowDesc
    {
        FE_STRUCT_RTTI(WindowDesc, "A32F5560-2333-4BE1-A661-59EAC29ABFBC");

        UInt32 Width{};
        UInt32 Height{};

        FE::StringSlice Title{};
    };

    class IWindow : public IObject
    {
    public:
        FE_CLASS_RTTI(IWindow, "2E09CD62-42A4-4E0D-BC2C-B11E849FBEAF");

        ~IWindow() override = default;

        virtual void PollEvents()         = 0;
        virtual bool CloseRequested()     = 0;
        virtual void* GetNativeHandle()   = 0;
        virtual Viewport CreateViewport() = 0;
        virtual Scissor CreateScissor()   = 0;
    };
} // namespace FE::GPU
