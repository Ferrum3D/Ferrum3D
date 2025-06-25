#include <FeCore/DI/Builder.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>

namespace FE::Graphics::Core
{
    void Module::RegisterServices(const DI::ServiceRegistryBuilder& builder)
    {
        FE_PROFILER_ZONE();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
        const Env::Configuration* config = serviceProvider->ResolveRequired<Env::Configuration>();
        const festd::string_view apiName = config->GetString("Graphics/Api", "Vulkan");
        if (apiName == "Vulkan")
        {
            const Rc deviceFactory = DI::DefaultNew<Vulkan::DeviceFactory>().value();
            deviceFactory->RegisterServices(builder);

            builder.Bind<DeviceFactory>().ToConst(deviceFactory.Get());
        }
        else
        {
            FE_AssertMsg(false, "Unknown graphics API:\"{}\"", apiName);
        }
    }

    FE_IMPLEMENT_MODULE(Module);
} // namespace FE::Graphics::Core
