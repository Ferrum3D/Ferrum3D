#include <FeCore/DI/Container.h>

namespace FE::DI
{
    Container::Container()
    {
        m_registryCallback.m_parent = this;
    }


    ResultCode Container::Resolve(const UUID registrationID, Memory::RefCountedObjectBase** ppResult)
    {
        FE_PROFILER_ZONE();

        ServiceRegistry* registry = nullptr;
        ServiceRegistration registration{};

        {
            const ServiceRegistryRoot::Reader registryReader = m_registryRoot.Read();
            for (ServiceRegistry& reg : registryReader)
            {
                if (const ServiceRegistration* foundRegistration = reg.FindByID(registrationID))
                {
                    registry = &reg;
                    registration = *foundRegistration;
                    break;
                }
            }
        }

        if (registry == nullptr)
            return ResultCode::kNotFound;

        std::lock_guard lk{ m_lock };

        LifetimeScope* lifetimeScope = registry->GetRootLifetimeScope();
        if (lifetimeScope == nullptr)
        {
            std::pmr::memory_resource* allocator = registry == m_registryRoot.GetRootRegistry()
                ? Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear)
                : std::pmr::get_default_resource();
            lifetimeScope = Memory::DefaultNew<LifetimeScope>(allocator, registry, this);
            registry->RegisterCallback(&m_registryCallback);
        }

        return lifetimeScope->Resolve(registration, ppResult);
    }
} // namespace FE::DI
