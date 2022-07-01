#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/SwapChain/ISwapChain.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT UInt32 ISwapChain_GetCurrentFrameIndex(ISwapChain* self)
        {
            return self->GetCurrentFrameIndex();
        }

        FE_DLL_EXPORT UInt32 ISwapChain_GetCurrentImageIndex(ISwapChain* self)
        {
            return self->GetCurrentImageIndex();
        }

        FE_DLL_EXPORT void ISwapChain_Present(ISwapChain* self)
        {
            self->Present();
        }

        FE_DLL_EXPORT IImageView* ISwapChain_GetDSV(ISwapChain* self)
        {
            auto result = self->GetDSV();
            return result.Detach();
        }

        FE_DLL_EXPORT void ISwapChain_GetRTVs(ISwapChain* self, IImageView** renderTargets, Int32* count)
        {
            auto result = self->GetRTVs();
            *count      = static_cast<Int32>(result.Size());
            if (renderTargets)
            {
                for (Int32 i = 0; i < result.Size(); ++i)
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
} // namespace FE::Osmium
