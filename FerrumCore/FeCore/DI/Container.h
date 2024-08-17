#pragma once
#include <FeCore/Containers/HashTables.h>
#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>
#include <FeCore/Parallel/Mutex.h>

namespace FE::DI
{
    class Container final : public IServiceProvider
    {
        struct CallbackImpl final : ServiceRegistryCallback
        {
            inline void OnDetach(ServiceRegistry* pRegistry) override
            {
                Container* pParent =
                    reinterpret_cast<Container*>(reinterpret_cast<uintptr_t>(this) - offsetof(Container, m_RegistryCallback));

                std::lock_guard lk{ pParent->m_Lock };
                LifetimeScope* pRootLifetimeScope = pRegistry->GetRootLifetimeScope();
                if (pRootLifetimeScope)
                    Memory::DefaultDelete(pRootLifetimeScope);
            }
        } m_RegistryCallback;

        Mutex m_Lock;
        ServiceRegistryRoot m_RegistryRoot;

    public:
        inline ServiceRegistryRoot* GetRegistryRoot()
        {
            return &m_RegistryRoot;
        }

        ResultCode Resolve(UUID registrationID, Memory::RefCountedObjectBase** ppResult) override;
    };
} // namespace FE::DI
