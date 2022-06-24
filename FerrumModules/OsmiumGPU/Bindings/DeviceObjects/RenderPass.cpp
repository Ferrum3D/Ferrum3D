#include <Bindings/DeviceObjects/RenderPass.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IRenderPass_Destruct(IRenderPass* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
