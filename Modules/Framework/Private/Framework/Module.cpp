#include <FeCore/Assets/IAssetManager.h>
#include <Framework/Application/Platform/Windows/PlatformApplication.h>
#include <Framework/Module.h>

namespace FE::Framework
{
    class FrameworkModuleImpl final : public ServiceLocatorImplBase<FrameworkModule>
    {
        Rc<Assets::IAssetManager> m_assetManager;

    public:
        FE_RTTI_Class(FrameworkModuleImpl, "3DD2CC5D-7629-4A44-A34A-5B84C9A80E95");

        void RegisterServices(DI::ServiceRegistryBuilder builder) override
        {
            FE_PROFILER_ZONE();

            builder.Bind<Core::PlatformApplication>().To<Windows::PlatformApplication>().InSingletonScope();
        }
    };

    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<FrameworkModuleImpl>();
    }
} // namespace FE::Framework
