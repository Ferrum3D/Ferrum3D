#include <OsGPU/Sampler/ISampler.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void ISampler_Destruct(ISampler* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
