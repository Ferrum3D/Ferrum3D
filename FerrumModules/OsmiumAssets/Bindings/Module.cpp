#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/OsmiumAssetsModule.h>

namespace FE::Osmium
{
    IFrameworkFactory* g_OsmiumAssetsModuleFactory;

    extern "C"
    {
        FE_DLL_EXPORT OsmiumAssetsModule* CreateModuleInstance(Env::Internal::IEnvironment* env)
        {
            Env::AttachEnvironment(*env);
            g_OsmiumAssetsModuleFactory = OsmiumAssetsModule::CreateFactory().Detach();
            g_OsmiumAssetsModuleFactory->Load();
            return ServiceLocator<OsmiumAssetsModule>::Get();
        }

        FE_DLL_EXPORT void DestructModuleInstance()
        {
            g_OsmiumAssetsModuleFactory->Unload();
            g_OsmiumAssetsModuleFactory->ReleaseStrongRef();
            g_OsmiumAssetsModuleFactory = nullptr;
        }

        FE_DLL_EXPORT void OsmiumAssetsModule_Initialize(OsmiumAssetsModule* self, OsmiumAssetsModuleDesc* desc)
        {
            self->Initialize(*desc);
        }
    }
} // namespace FE::Osmium
