#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Images/ImageAssetStorage.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT ImageAssetStorage* ImageAssetStorage_Load(Assets::IAssetManager* manager, const GUID* assetID)
        {
            return fe_assert_cast<ImageAssetStorage*>(manager->LoadAsset(UUID::FromGUID(*assetID)));
        }

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

        FE_DLL_EXPORT void ImageAssetStorage_Destruct(ImageAssetStorage* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
