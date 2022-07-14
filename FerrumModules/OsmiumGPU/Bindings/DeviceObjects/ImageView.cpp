#include <OsGPU/ImageView/IImageView.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IImageView_GetDesc(IImageView* self, ImageViewDesc* desc)
        {
            *desc = self->GetDesc();
        }

        FE_DLL_EXPORT void IImageView_Destruct(IImageView* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
