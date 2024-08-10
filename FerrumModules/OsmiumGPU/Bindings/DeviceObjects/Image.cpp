#include <OsGPU/Image/IImage.h>
#include <OsGPU/ImageView/IImageView.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IImage_GetDesc(IImage* self, ImageDesc* desc)
        {
            *desc = self->GetDesc();
        }

        FE_DLL_EXPORT IImageView* IImage_CreateView(IImage* self, ImageAspectFlags aspectFlags)
        {
            return self->CreateView(aspectFlags).Detach();
        }

        FE_DLL_EXPORT void IImage_AllocateMemory(IImage* self, int32_t memoryType)
        {
            self->AllocateMemory(static_cast<MemoryType>(memoryType));
        }

        FE_DLL_EXPORT void IImage_Destruct(IImage* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
