#include <Core/DI/Builder.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>

namespace FE::Graphics::Core
{
    void Module::RegisterServices(const DI::ServiceRegistryBuilder& builder)
    {
        FE_PROFILER_ZONE();

        const Rc deviceFactory = DI::DefaultNew<Vulkan::DeviceFactory>().value();
        deviceFactory->RegisterServices(builder);

        builder.Bind<DeviceFactory>().ToConst(deviceFactory.Get());
    }

    FE_IMPLEMENT_MODULE(Module);
} // namespace FE::Graphics::Core
