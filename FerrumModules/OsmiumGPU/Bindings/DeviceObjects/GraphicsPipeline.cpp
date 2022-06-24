#include <Bindings/DeviceObjects/GraphicsPipeline.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IGraphicsPipeline_Destruct(IGraphicsPipeline* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
