#include <Bindings/DeviceObjects/Framebuffer.h>
#include <OsGPU/Framebuffer/IFramebuffer.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IFramebuffer_Destruct(IFramebuffer* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
