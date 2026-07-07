#include <Core/DI/Builder.h>
#include <Core/DI/Registry.h>
#include <Core/IO/AsyncStreamIO.h>

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
        builder.Bind<IO::IAsyncStreamIO>().To<IO::AsyncStreamIO>().InSingletonScope();
    }


    void ServiceRegistryBuilder::Build()
    {
        m_registry->Sort();
        m_registry.Reset();
    }
} // namespace FE::DI
