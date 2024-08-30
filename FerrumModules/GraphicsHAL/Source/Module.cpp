#include <HAL/Module.h>
#include <HAL/Vulkan/DeviceFactory.h>
#include <HAL/Window.h>

namespace FE::Graphics::HAL
{
    class OsmiumGPUModuleImpl final : public ServiceLocatorImplBase<OsmiumGPUModule>
    {
        Rc<DeviceFactory> m_pDeviceFactory;

    public:
        FE_RTTI_Class(OsmiumGPUModuleImpl, "CB3A80B7-EED3-4FBF-8694-1ED61246234A");

        OsmiumGPUModuleImpl()
        {
            ModuleBase::Initialize();
        }

        ~OsmiumGPUModuleImpl() override = default;

        inline void RegisterServices(DI::ServiceRegistryBuilder builder) override
        {
            builder.Bind<IWindow>().To<Window>().InSingletonScope();

            DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
            Env::Configuration* pConfig = pServiceProvider->ResolveRequired<Env::Configuration>();
            Logger* pLogger = pServiceProvider->ResolveRequired<Logger>();
            const StringSlice apiName = pConfig->GetString("Graphics/Api", "Vulkan");
            if (apiName == "Vulkan")
            {
                Rc deviceFactory = Rc<Vulkan::DeviceFactory>::DefaultNew(pConfig, pLogger);
                deviceFactory->RegisterServices(builder);

                m_pDeviceFactory = deviceFactory;
                builder.Bind<DeviceFactory>().ToConst(deviceFactory.Get());
            }
            else
            {
                FE_AssertMsg(false, "Unknown graphics API:\"{}\"", apiName);
            }
        }
    };


    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** ppModule)
    {
        Env::AttachEnvironment(environment);
        *ppModule = Memory::DefaultNew<OsmiumGPUModuleImpl>();
    }
} // namespace FE::Graphics::HAL
