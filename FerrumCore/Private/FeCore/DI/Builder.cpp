#include <FeCore/DI/Builder.h>
#include <FeCore/DI/Registry.h>

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


    void ServiceRegistryBuilder::Build()
    {
        m_registry->Sort();
        m_registry.Reset();
    }
} // namespace FE::DI
