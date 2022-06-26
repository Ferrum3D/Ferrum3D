#include <FeCore/Assets/AssetRegistry.h>

namespace FE::Assets
{
    extern "C"
    {
        FE_DLL_EXPORT AssetRegistry* AssetRegistry_Construct()
        {
            return MakeShared<AssetRegistry>().Detach();
        }

        FE_DLL_EXPORT void AssetRegistry_LoadAssetsFromFile(AssetRegistry* self, const char* fileName)
        {
            self->LoadAssetsFromFile(fileName);
        }

        FE_DLL_EXPORT void AssetRegistry_Destruct(AssetRegistry* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Assets
