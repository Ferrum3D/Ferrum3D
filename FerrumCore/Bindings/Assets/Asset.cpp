#include <FeCore/Assets/Asset.h>

namespace FE::Assets
{
    extern "C"
    {
        FE_DLL_EXPORT void Asset_AddStrongRef(Asset<AssetStorage>* self)
        {
            if (self->Get())
            {
                self->Get()->AddStrongRef();
            }
        }

        FE_DLL_EXPORT void Asset_ReleaseStrongRef(Asset<AssetStorage>* self)
        {
            if (self->Get())
            {
                self->Get()->ReleaseStrongRef();
            }
        }

        FE_DLL_EXPORT void Asset_LoadSync(Asset<AssetStorage>* self)
        {
            self->LoadSync();
        }
    }
} // namespace FE::Assets
