#include <GPU/SwapChain/ISwapChain.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void ISwapChain_Destruct(ISwapChain* self)
        {
            self->ReleaseStrongRef();
        }
    }
}
