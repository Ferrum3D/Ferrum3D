#include <Graphics/Core/Module.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>

namespace FE::Graphics::Core
{
    class GraphicsCoreModuleImpl final : public GraphicsCoreModule
    {
        DeviceFactory* m_deviceFactory = nullptr;

    public:
        FE_RTTI_Class(GraphicsCoreModuleImpl, "CB3A80B7-EED3-4FBF-8694-1ED61246234A");

        void RegisterServices(DI::ServiceRegistryBuilder builder) override
        {
            FE_PROFILER_ZONE();

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
            const Env::Configuration* config = serviceProvider->ResolveRequired<Env::Configuration>();
            const festd::string_view apiName = config->GetString("Graphics/Api", "Vulkan");
            if (apiName == "Vulkan")
            {
                const Rc deviceFactory = DI::DefaultNew<Vulkan::DeviceFactory>().value();
                deviceFactory->RegisterServices(builder);

                m_deviceFactory = deviceFactory.Get();
                builder.Bind<DeviceFactory>().ToConst(deviceFactory.Get());
            }
            else
            {
                FE_AssertMsg(false, "Unknown graphics API:\"{}\"", apiName);
            }
        }
    };


    extern "C" FE_DLL_EXPORT void CreateModuleInstance(const Env::EnvironmentHandle environment, IModule** module)
    {
        FE_PROFILER_ZONE();

        Env::AttachEnvironment(environment);
        *module = Memory::DefaultNew<GraphicsCoreModuleImpl>();
    }
} // namespace FE::Graphics::Core
