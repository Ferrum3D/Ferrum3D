#include <Bindings/DeviceObjects/Framebuffer.h>
#include <GPU/Framebuffer/IFramebuffer.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IFramebuffer_Destruct(IFramebuffer* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
