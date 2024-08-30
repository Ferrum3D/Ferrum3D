#include <FeCore/Assets/IAssetManager.h>
#include <Graphics/Assets/ImageAssetLoader.h>
#include <Graphics/Assets/MeshAssetLoader.h>
#include <Graphics/Assets/ShaderAssetLoader.h>
#include <Graphics/Module.h>

namespace FE::Graphics
{
    class OsmiumAssetsModuleImpl : public ServiceLocatorImplBase<OsmiumAssetsModule>
    {
        Rc<Assets::IAssetManager> m_pAssetManager;

    public:
        FE_RTTI_Class(OsmiumAssetsModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        OsmiumAssetsModuleImpl()
        {
            ModuleBase::Initialize();
        }

        inline ~OsmiumAssetsModuleImpl() override
        {
            m_pAssetManager->UnregisterAssetLoader(Env::Name{ ImageAssetStorage::kAssetTypeName });
            m_pAssetManager->UnregisterAssetLoader(Env::Name{ MeshAssetStorage::kAssetTypeName });
            m_pAssetManager->UnregisterAssetLoader(Env::Name{ ShaderAssetStorage::kAssetTypeName });
        }

        inline void RegisterServices(DI::ServiceRegistryBuilder) override
        {
            // TODO:
            //   Maybe register multiple instances for a single IAssetLoader interface.
            //   The loaders will automatically unregister when the module detaches.
            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            m_pAssetManager = pServiceProvider->ResolveRequired<Assets::IAssetManager>();
            m_pAssetManager->RegisterAssetLoader(DI::DefaultNew<ImageAssetLoader>().Unwrap());
            m_pAssetManager->RegisterAssetLoader(DI::DefaultNew<MeshAssetLoader>().Unwrap());
            m_pAssetManager->RegisterAssetLoader(DI::DefaultNew<ShaderAssetLoader>().Unwrap());
        }
    };

    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<OsmiumAssetsModuleImpl>();
    }
} // namespace FE::Graphics
