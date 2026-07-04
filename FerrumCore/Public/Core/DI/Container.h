#pragma once
#include <Core/DI/LifetimeScope.h>
#include <Core/DI/Registry.h>
#include <Core/Threading/Mutex.h>
#include <festd/unordered_map.h>

namespace FE::DI
{
    struct Container final : public IServiceProvider
    {
        FE_RTTI("60933641-C6F7-4503-8FFA-AC25EDFAD83F");

        Container();

        ServiceRegistryRoot* GetRegistryRoot()
        {
            return &m_registryRoot;
        }

        ResultCode Resolve(Uuid registrationID, Memory::RefCountedObjectBase** ppResult) override;

    private:
        struct CallbackImpl final : public ServiceRegistryCallback
        {
            void OnDetach(ServiceRegistry* registry) override
            {
                std::lock_guard lk{ m_parent->m_lock };
                if (LifetimeScope* rootLifetimeScope = registry->GetRootLifetimeScope())
                    Memory::DefaultDelete(rootLifetimeScope);
            }

            Container* m_parent = nullptr;
        } m_registryCallback;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        Threading::Mutex m_lock;
        ServiceRegistryRoot m_registryRoot;
    };
} // namespace FE::DI
