#include <FeCore/Assets/IAssetManager.h>
#include <Graphics/Assets/TextureAssetManager.h>
#include <Graphics/Module.h>

namespace FE::Graphics
{
    class GraphicsModuleImpl final : public ServiceLocatorImplBase<GraphicsModule>
    {
    public:
        FE_RTTI_Class(GraphicsModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        void RegisterServices(DI::ServiceRegistryBuilder builder) override
        {
            builder.Bind<ITextureAssetManager>().To<TextureAssetManager>().InSingletonScope();
        }
    };

    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<GraphicsModuleImpl>();
    }
} // namespace FE::Graphics
