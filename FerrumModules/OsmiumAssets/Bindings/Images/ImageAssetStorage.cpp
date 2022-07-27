#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Images/ImageAssetStorage.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT const UInt8* ImageAssetStorage_Data(ImageAssetStorage* self)
        {
            return self->Data();
        }

        FE_DLL_EXPORT Int32 ImageAssetStorage_Width(ImageAssetStorage* self)
        {
            return self->Width();
        }

        FE_DLL_EXPORT Int32 ImageAssetStorage_Height(ImageAssetStorage* self)
        {
            return self->Height();
        }

        FE_DLL_EXPORT USize ImageAssetStorage_Size(ImageAssetStorage* self)
        {
            return self->Size();
        }
    }
} // namespace FE::Osmium
