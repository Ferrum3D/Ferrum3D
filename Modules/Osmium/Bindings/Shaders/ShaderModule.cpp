#include <Bindings/Shaders/ShaderModule.h>
#include <GPU/Shader/IShaderModule.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IShaderModule_Destruct(IShaderModule* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
