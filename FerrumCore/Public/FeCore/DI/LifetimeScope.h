#pragma once
#include <FeCore/DI/Registration.h>
#include <FeCore/DI/Registry.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <festd/unordered_map.h>

namespace FE::DI
{
    struct LifetimeScope final
    {
        LifetimeScope(std::pmr::memory_resource* pAllocator, ServiceRegistry* pRegistry, IServiceProvider* pRootProvider)
            : m_table(pAllocator)
            , m_registry(pRegistry)
            , m_rootProvider(pRootProvider)
        {
            if (pRegistry->m_rootLifetimeScope == nullptr)
                pRegistry->m_rootLifetimeScope = this;
        }

        ~LifetimeScope();

        ResultCode Resolve(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);

    private:
        festd::pmr::segmented_unordered_dense_map<ServiceRegistration, Memory::RefCountedObjectBase*> m_table;
        ServiceRegistry* m_registry;
        IServiceProvider* m_rootProvider;

        ResultCode ActivateImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);
        ResultCode ResolveImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);
    };
} // namespace FE::DI
