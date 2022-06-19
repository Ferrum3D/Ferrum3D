#include <GPU/SwapChain/ISwapChain.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void ISwapChain_GetRTVs(ISwapChain* self, IImageView** renderTargets, Int32* count)
        {
            auto result = self->GetRTVs();
            *count = static_cast<Int32>(result.size());
            if (renderTargets)
            {
                for (Int32 i = 0; i < result.size(); ++i)
                {
                    renderTargets[i] = result[i].Detach();
                }
            }
        }

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
