#include <Core/DI/Builder.h>
#include <Core/DI/Registry.h>

#include <Core/IO/AsyncStreamIO.h>
#include <Core/IO/StreamFactory.h>
#include <Core/Jobs/JobSystem.h>
#include <Core/Logging/Logger.h>

namespace FE::DI
{
    ServiceRegistryBuilder::ServiceRegistryBuilder(ServiceRegistry* pRegistry)
        : m_registry(pRegistry)
    {
    }


    Internal::ServiceRegistrationSpec ServiceRegistryBuilder::BindImpl(const Uuid& id) const
    {
        ServiceRegistration* pRegistration = m_registry->Add(id);
        ServiceActivator* pActivator = m_registry->GetActivator(pRegistration->m_index);
        return { pRegistration, pActivator };
    }


    void RegisterCoreServices(const ServiceRegistryBuilder& builder)
    {
        builder.Bind<IJobSystem>().To<JobSystem>().InSingletonScope();
        builder.Bind<Logger>().ToSelf().InSingletonScope();
        builder.Bind<IO::IStreamFactory>().To<IO::FileStreamFactory>().InSingletonScope();
#if FE_PLATFORM_WINDOWS
        builder.Bind<IO::IAsyncIOBackend>().To<IO::OverlappedAsyncIOBackend>().InSingletonScope();
#else
        builder.Bind<IO::IAsyncIOBackend>().To<IO::DefaultAsyncIOBackend>().InSingletonScope();
#endif
        builder.Bind<IO::IAsyncStreamIO>().To<IO::AsyncStreamIO>().InSingletonScope();
    }


    void ServiceRegistryBuilder::Build()
    {
        m_registry->Sort();
        m_registry.Reset();
    }
} // namespace FE::DI
