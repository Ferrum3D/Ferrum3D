#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/OsmiumAssetsModule.h>
#include <OsAssets/Shaders/ShaderAssetLoader.h>

namespace FE::Osmium
{
    void OsmiumAssetsModule::GetFrameworkDependencies(List<Shared<IFrameworkFactory>>& /* dependencies */) {}

    class OsmiumAssetsModuleImpl : public ServiceLocatorImplBase<OsmiumAssetsModule>
    {
        OsmiumAssetsModuleDesc m_Desc;

    public:
        FE_CLASS_RTTI(OsmiumAssetsModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        OsmiumAssetsModuleImpl();

        ~OsmiumAssetsModuleImpl() override
        {
            auto* manager = ServiceLocator<Assets::IAssetManager>::Get();
            manager->RemoveAssetLoader(ImageAssetLoader::AssetType);
            manager->RemoveAssetLoader(MeshAssetLoader::AssetType);
            manager->RemoveAssetLoader(ShaderAssetLoader::AssetType);
        }

        void Initialize(const OsmiumAssetsModuleDesc& desc) override
        {
            FrameworkBase::Initialize();
            m_Desc = desc;

            auto* manager = ServiceLocator<Assets::IAssetManager>::Get();
            manager->RegisterAssetLoader(MakeShared<ImageAssetLoader>());
            manager->RegisterAssetLoader(MakeShared<MeshAssetLoader>());
            manager->RegisterAssetLoader(MakeShared<ShaderAssetLoader>());
        }
    };

    OsmiumAssetsModuleImpl::OsmiumAssetsModuleImpl()
    {
        SetInfo(ModuleInfo("Osmium.Assets", "Osmium's asset loading module", "Ferrum3D"));
    }

    extern "C" FE_DLL_EXPORT OsmiumAssetsModule* CreateModuleInstance(Env::Internal::IEnvironment* environment)
    {
        Env::AttachEnvironment(*environment);
        return MakeShared<OsmiumAssetsModuleImpl>().Detach();
    }
} // namespace FE::Osmium
