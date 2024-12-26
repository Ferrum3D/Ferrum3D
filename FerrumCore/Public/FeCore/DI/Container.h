#pragma once
#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>
#include <FeCore/Parallel/Mutex.h>
#include <festd/unordered_map.h>

namespace FE::DI
{
    struct Container final : public IServiceProvider
    {
        Container();

        ServiceRegistryRoot* GetRegistryRoot()
        {
            return &m_registryRoot;
        }

        ResultCode Resolve(UUID registrationID, Memory::RefCountedObjectBase** ppResult) override;

    private:
        struct CallbackImpl final : public ServiceRegistryCallback
        {
            void OnDetach(ServiceRegistry* pRegistry) override
            {
                std::lock_guard lk{ m_parent->m_lock };
                if (LifetimeScope* rootLifetimeScope = pRegistry->GetRootLifetimeScope())
                    Memory::DefaultDelete(rootLifetimeScope);
            }

            Container* m_parent = nullptr;
        } m_registryCallback;

        Mutex m_lock;
        ServiceRegistryRoot m_registryRoot;
    };
} // namespace FE::DI
