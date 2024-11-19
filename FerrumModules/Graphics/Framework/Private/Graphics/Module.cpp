#include <FeCore/Assets/IAssetManager.h>
#include <Graphics/Assets/ImageAssetLoader.h>
#include <Graphics/Assets/MeshAssetLoader.h>
#include <Graphics/Assets/ShaderAssetLoader.h>
#include <Graphics/Module.h>

namespace FE::Graphics
{
    class GraphicsModuleImpl final : public ServiceLocatorImplBase<GraphicsModule>
    {
        Rc<Assets::IAssetManager> m_assetManager;

    public:
        FE_RTTI_Class(GraphicsModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        GraphicsModuleImpl()
        {
            ModuleBase::Initialize();
        }

        ~GraphicsModuleImpl() override
        {
            m_assetManager->UnregisterAssetLoader(Env::Name{ ImageAssetStorage::kAssetTypeName });
            m_assetManager->UnregisterAssetLoader(Env::Name{ MeshAssetStorage::kAssetTypeName });
            m_assetManager->UnregisterAssetLoader(Env::Name{ ShaderAssetStorage::kAssetTypeName });
        }

        void RegisterServices(DI::ServiceRegistryBuilder) override
        {
            // TODO:
            //   Maybe register multiple instances for a single IAssetLoader interface.
            //   The loaders will automatically unregister when the module detaches.
            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            m_assetManager = pServiceProvider->ResolveRequired<Assets::IAssetManager>();
            m_assetManager->RegisterAssetLoader(DI::DefaultNew<ImageAssetLoader>().Unwrap());
            m_assetManager->RegisterAssetLoader(DI::DefaultNew<MeshAssetLoader>().Unwrap());
            m_assetManager->RegisterAssetLoader(DI::DefaultNew<ShaderAssetLoader>().Unwrap());
        }
    };

    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<GraphicsModuleImpl>();
    }
} // namespace FE::Graphics
