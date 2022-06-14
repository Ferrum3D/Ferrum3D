#include <Bindings/WindowSystem/Window.h>

namespace FE::GPU
{
    extern "C"
    {
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
}
