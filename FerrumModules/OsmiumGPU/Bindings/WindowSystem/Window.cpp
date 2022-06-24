#include <Bindings/WindowSystem/Window.h>
#include <GPU/Window/IWindow.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void* IWindow_GetNativeHandle(IWindow* self)
        {
            return self->GetNativeHandle();
        }

        FE_DLL_EXPORT void IWindow_CreateViewport(IWindow* self, Viewport* viewport)
        {
            *viewport = self->CreateViewport();
        }

        FE_DLL_EXPORT void IWindow_CreateScissor(IWindow* self, Scissor* scissor)
        {
            *scissor = self->CreateScissor();
        }

        FE_DLL_EXPORT void IWindow_PollEvents(IWindow* self)
        {
            self->PollEvents();
        }

        FE_DLL_EXPORT bool IWindow_CloseRequested(IWindow* self)
        {
            return self->CloseRequested();
        }

        FE_DLL_EXPORT void IWindow_Destruct(IWindow* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
