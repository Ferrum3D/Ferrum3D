#include <Graphics/RHI/Module.h>
#include <Graphics/RHI/Vulkan/DeviceFactory.h>
#include <Graphics/RHI/Window.h>

namespace FE::Graphics::RHI
{
    class GraphicsRHIModuleImpl final : public ServiceLocatorImplBase<GraphicsRHIModule>
    {
        Rc<DeviceFactory> m_deviceFactory;

    public:
        FE_RTTI_Class(GraphicsRHIModuleImpl, "CB3A80B7-EED3-4FBF-8694-1ED61246234A");

        GraphicsRHIModuleImpl()
        {
            ModuleBase::Initialize();
        }

        ~GraphicsRHIModuleImpl() override = default;

        inline void RegisterServices(DI::ServiceRegistryBuilder builder) override
        {
            builder.Bind<IWindow>().To<Window>().InSingletonScope();

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
            Env::Configuration* config = serviceProvider->ResolveRequired<Env::Configuration>();
            Logger* logger = serviceProvider->ResolveRequired<Logger>();
            const StringSlice apiName = config->GetString("Graphics/Api", "Vulkan");
            if (apiName == "Vulkan")
            {
                Rc deviceFactory = Rc<Vulkan::DeviceFactory>::DefaultNew(config, logger);
                deviceFactory->RegisterServices(builder);

                m_deviceFactory = deviceFactory;
                builder.Bind<DeviceFactory>().ToConst(deviceFactory.Get());
            }
            else
            {
                FE_AssertMsg(false, "Unknown graphics API:\"{}\"", apiName);
            }
        }
    };


    extern "C" FE_DLL_EXPORT void CreateModuleInstance(Env::Internal::IEnvironment& environment, IModule** module)
    {
        Env::AttachEnvironment(environment);
        *module = Memory::DefaultNew<GraphicsRHIModuleImpl>();
    }
} // namespace FE::Graphics::RHI
