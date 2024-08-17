#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/OsmiumAssetsModule.h>
#include <OsAssets/Shaders/ShaderAssetLoader.h>

namespace FE::Osmium
{
    class OsmiumAssetsModuleImpl : public ServiceLocatorImplBase<OsmiumAssetsModule>
    {
        Rc<Assets::IAssetManager> m_pAssetManager;

    public:
        FE_RTTI_Class(OsmiumAssetsModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        OsmiumAssetsModuleImpl()
        {
            // TODO:
            //   Maybe register multiple instances for a single IAssetLoader interface.
            //   The loaders will automatically unregister when the module detaches.
            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            m_pAssetManager = pServiceProvider->ResolveRequired<Assets::IAssetManager>();
            m_pAssetManager->RegisterAssetLoader(Rc<ImageAssetLoader>::DefaultNew());
            m_pAssetManager->RegisterAssetLoader(Rc<MeshAssetLoader>::DefaultNew());
            m_pAssetManager->RegisterAssetLoader(Rc<ShaderAssetLoader>::DefaultNew());

            ModuleBase::Initialize();
        }

        inline ~OsmiumAssetsModuleImpl() override
        {
            m_pAssetManager->RemoveAssetLoader(ImageAssetLoader::AssetType);
            m_pAssetManager->RemoveAssetLoader(MeshAssetLoader::AssetType);
            m_pAssetManager->RemoveAssetLoader(ShaderAssetLoader::AssetType);
        }

        inline void RegisterServices(DI::ServiceRegistryBuilder) override {}
    };

    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<OsmiumAssetsModuleImpl>();
    }
} // namespace FE::Osmium
