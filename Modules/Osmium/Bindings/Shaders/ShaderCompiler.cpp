#include <GPU/Shader/IShaderCompiler.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IShaderCompiler_Destruct(IShaderCompiler* self)
        {
            self->ReleaseStrongRef();
        }
    }
}
