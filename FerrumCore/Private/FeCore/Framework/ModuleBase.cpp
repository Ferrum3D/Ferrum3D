#include <FeCore/Framework/ModuleBase.h>
#include <FeCore/Modules/EnvironmentPrivate.h>

namespace FE
{
    ModuleBase::ModuleBase()
        : m_pRegistry(Env::Internal::CreateServiceRegistry())
    {
    }


    ModuleBase::~ModuleBase() = default;


    void ModuleBase::Initialize()
    {
        FE_PROFILER_FUNCTION();
        DI::ServiceRegistryBuilder builder{ m_pRegistry.Get() };
        RegisterServices(builder);
        builder.Build();
    }
} // namespace FE
