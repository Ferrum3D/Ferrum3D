#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Modules/ModuleBase.h>

namespace FE
{
    ModuleBase::ModuleBase()
        : m_pRegistry(Env::CreateServiceRegistry())
    {
    }


    ModuleBase::~ModuleBase() = default;


    void ModuleBase::Initialize()
    {
        FE_PROFILER_ZONE();
        DI::ServiceRegistryBuilder builder{ m_pRegistry.Get() };
        RegisterServices(builder);
        builder.Build();
    }
} // namespace FE
