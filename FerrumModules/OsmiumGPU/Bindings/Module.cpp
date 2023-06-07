#include <OsGPU/OsmiumGPUModule.h>

namespace FE::Osmium
{
    IFrameworkFactory* g_OsmiumGPUModuleFactory;

    extern "C"
    {
        FE_DLL_EXPORT OsmiumGPUModule* CreateModuleInstance(Env::Internal::IEnvironment* env)
        {
            Env::AttachEnvironment(*env);
            g_OsmiumGPUModuleFactory = OsmiumGPUModule::CreateFactory().Detach();
            g_OsmiumGPUModuleFactory->Load();
            return ServiceLocator<OsmiumGPUModule>::Get();
        }

        FE_DLL_EXPORT void DestructModuleInstance()
        {
            g_OsmiumGPUModuleFactory->Unload();
            g_OsmiumGPUModuleFactory->ReleaseStrongRef();
            g_OsmiumGPUModuleFactory = nullptr;
        }

        FE_DLL_EXPORT void OsmiumGPUModule_Initialize(OsmiumGPUModule* self, OsmiumGPUModuleDesc* desc)
        {
            self->Initialize(*desc);
        }

        FE_DLL_EXPORT IInstance* OsmiumGPUModule_CreateInstance(OsmiumGPUModule* self)
        {
            return self->CreateInstance().Detach();
        }
    }
} // namespace FE::Osmium
