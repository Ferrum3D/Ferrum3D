#include <GPU/SwapChain/ISwapChain.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void ISwapChain_GetDesc(ISwapChain* self, SwapChainDesc* desc)
        {
            *desc = self->GetDesc();
        }

        FE_DLL_EXPORT void ISwapChain_Destruct(ISwapChain* self)
        {
            self->ReleaseStrongRef();
        }
    }
}
