#include <GPU/ImageView/IImageView.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IImageView_Destruct(IImageView* self)
        {
            self->ReleaseStrongRef();
        }
    }
}
