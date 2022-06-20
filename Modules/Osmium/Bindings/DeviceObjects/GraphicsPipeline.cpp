#include <Bindings/DeviceObjects/GraphicsPipeline.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IGraphicsPipeline_Destruct(IGraphicsPipeline* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
