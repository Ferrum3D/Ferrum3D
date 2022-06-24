#include <Bindings/Shaders/ShaderModule.h>
#include <OsGPU/Shader/IShaderModule.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IShaderModule_Destruct(IShaderModule* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
