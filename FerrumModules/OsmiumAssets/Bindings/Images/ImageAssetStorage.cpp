#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Images/ImageAssetStorage.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT const uint8_t* ImageAssetStorage_Data(ImageAssetStorage* self)
        {
            return self->Data();
        }

        FE_DLL_EXPORT int32_t ImageAssetStorage_Width(ImageAssetStorage* self)
        {
            return self->Width();
        }

        FE_DLL_EXPORT int32_t ImageAssetStorage_Height(ImageAssetStorage* self)
        {
            return self->Height();
        }

        FE_DLL_EXPORT size_t ImageAssetStorage_Size(ImageAssetStorage* self)
        {
            return self->Size();
        }
    }
} // namespace FE::Osmium
