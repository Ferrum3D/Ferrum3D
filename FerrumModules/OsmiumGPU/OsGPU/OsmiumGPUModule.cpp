#include <OsGPU/Instance/VKInstance.h>
#include <OsGPU/OsmiumGPUModule.h>

namespace FE::Osmium
{
    class OsmiumGPUModuleImpl final : public ServiceLocatorImplBase<OsmiumGPUModule>
    {
    public:
        FE_RTTI_Class(OsmiumGPUModuleImpl, "CB3A80B7-EED3-4FBF-8694-1ED61246234A");

        OsmiumGPUModuleImpl()
        {
            ModuleBase::Initialize();
        }

        ~OsmiumGPUModuleImpl() override = default;

        inline void RegisterServices(DI::ServiceRegistryBuilder builder) override
        {
            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            Env::Configuration* pConfig = pServiceProvider->ResolveRequired<Env::Configuration>();
            const StringSlice apiName = pConfig->GetString("Graphics/Api", "Vulkan");
            if (apiName == "Vulkan")
            {
                builder.Bind<IInstance>().To<VKInstance>().InSingletonScope();
            }
            else
            {
                FE_UNREACHABLE("Unknown graphics API:\"{}\"", apiName);
            }
        }
    };


    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<OsmiumGPUModuleImpl>();
    }
} // namespace FE::Osmium
