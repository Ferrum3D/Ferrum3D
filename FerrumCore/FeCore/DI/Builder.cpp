#include <FeCore/DI/Builder.h>
#include <FeCore/DI/Registry.h>

namespace FE::DI
{
    ServiceRegistryBuilder::ServiceRegistryBuilder(ServiceRegistry* pRegistry)
        : m_pRegistry(pRegistry)
    {
    }


    Internal::ServiceRegistrationSpec ServiceRegistryBuilder::BindImpl(const UUID& id)
    {
        ServiceRegistration* pRegistration = m_pRegistry->Add(id);
        ServiceActivator* pActivator = m_pRegistry->GetActivator(pRegistration->GetIndex());
        return { pRegistration, pActivator };
    }


    void ServiceRegistryBuilder::Build()
    {
        m_pRegistry->Sort();
        m_pRegistry.Reset();
    }
} // namespace FE::DI
