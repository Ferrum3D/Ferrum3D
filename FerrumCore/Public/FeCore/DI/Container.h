#pragma once
#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>
#include <FeCore/Parallel/Mutex.h>
#include <festd/unordered_map.h>

namespace FE::DI
{
    struct Container final : public IServiceProvider
    {
        ServiceRegistryRoot* GetRegistryRoot()
        {
            return &m_registryRoot;
        }

        ResultCode Resolve(UUID registrationID, Memory::RefCountedObjectBase** ppResult) override;

    private:
        struct CallbackImpl final : ServiceRegistryCallback
        {
            void OnDetach(ServiceRegistry* pRegistry) override
            {
                FE_PUSH_CLANG_WARNING("-Winvalid-offsetof")
                Container* pParent =
                    reinterpret_cast<Container*>(reinterpret_cast<uintptr_t>(this) - offsetof(Container, m_registryCallback));
                FE_POP_CLANG_WARNING

                std::lock_guard lk{ pParent->m_lock };
                LifetimeScope* pRootLifetimeScope = pRegistry->GetRootLifetimeScope();
                if (pRootLifetimeScope)
                    Memory::DefaultDelete(pRootLifetimeScope);
            }
        } m_registryCallback;

        Mutex m_lock;
        ServiceRegistryRoot m_registryRoot;
    };
} // namespace FE::DI
