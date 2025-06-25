#include <FeCore/DI/Builder.h>
#include <Framework/Application/Platform/Windows/PlatformApplication.h>
#include <Framework/Module.h>

namespace FE::Framework
{
    void Module::RegisterServices(const DI::ServiceRegistryBuilder& builder)
    {
        FE_PROFILER_ZONE();

        builder.Bind<Core::PlatformApplication>().To<Windows::PlatformApplication>().InSingletonScope();
    }

    FE_IMPLEMENT_MODULE(Module);
} // namespace FE::Framework
