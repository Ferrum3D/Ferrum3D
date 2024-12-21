#include <FeCore/DI/Container.h>

namespace FE::DI
{
    ResultCode Container::Resolve(UUID registrationID, Memory::RefCountedObjectBase** ppResult)
    {
        ServiceRegistry* pRegistry = nullptr;
        ServiceRegistration registration{};

        {
            const ServiceRegistryRoot::Reader registryReader = m_registryRoot.Read();
            for (ServiceRegistry& registry : registryReader)
            {
                if (const ServiceRegistration* pRegistration = registry.FindByID(registrationID))
                {
                    pRegistry = &registry;
                    registration = *pRegistration;
                    break;
                }
            }
        }

        if (pRegistry == nullptr)
        {
            return ResultCode::kNotFound;
        }

        std::lock_guard lk{ m_lock };

        LifetimeScope* pLifetimeScope = pRegistry->GetRootLifetimeScope();
        if (pLifetimeScope == nullptr)
        {
            std::pmr::memory_resource* pAllocator = pRegistry == m_registryRoot.GetRootRegistry()
                ? Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear)
                : std::pmr::get_default_resource();
            pLifetimeScope = Memory::DefaultNew<LifetimeScope>(pAllocator, pRegistry, this);
            pRegistry->RegisterCallback(&m_registryCallback);
        }

        return pLifetimeScope->Resolve(registration, ppResult);
    }
} // namespace FE::DI
