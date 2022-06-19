#include <Bindings/DeviceObjects/Framebuffer.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IFramebuffer_Destruct(IFramebuffer* self)
        {
            self->ReleaseStrongRef();
        }
    }
}
