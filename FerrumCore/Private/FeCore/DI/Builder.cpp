#include <FeCore/DI/Builder.h>
#include <FeCore/DI/Registry.h>

#include <FeCore/IO/AsyncStreamIO.h>
#include <FeCore/IO/StreamFactory.h>
#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Logging/Logger.h>


namespace FE::DI
{
    ServiceRegistryBuilder::ServiceRegistryBuilder(ServiceRegistry* pRegistry)
        : m_registry(pRegistry)
    {
    }


    Internal::ServiceRegistrationSpec ServiceRegistryBuilder::BindImpl(const UUID& id)
    {
        ServiceRegistration* pRegistration = m_registry->Add(id);
        ServiceActivator* pActivator = m_registry->GetActivator(pRegistration->GetIndex());
        return { pRegistration, pActivator };
    }


    void RegisterCoreServices(ServiceRegistryBuilder builder)
    {
        builder.Bind<IJobSystem>().To<JobSystem>().InSingletonScope();
        builder.Bind<Logger>().ToSelf().InSingletonScope();
        builder.Bind<IO::IStreamFactory>().To<IO::FileStreamFactory>().InSingletonScope();
        builder.Bind<IO::IAsyncStreamIO>().To<IO::AsyncStreamIO>().InSingletonScope();
    }


    void ServiceRegistryBuilder::Build()
    {
        m_registry->Sort();
        m_registry.Reset();
    }
} // namespace FE::DI
