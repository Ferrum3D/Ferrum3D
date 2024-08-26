#pragma once
#include <FeCore/Containers/HashTables.h>
#include <FeCore/DI/Registration.h>
#include <FeCore/DI/Registry.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::DI
{
    class LifetimeScope final
    {
        festd::pmr::segmented_unordered_dense_map<ServiceRegistration, Memory::RefCountedObjectBase*> m_Table;
        ServiceRegistry* m_pRegistry;
        IServiceProvider* m_pRootProvider;

        ResultCode ActivateImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);
        ResultCode ResolveImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);

    public:
        inline LifetimeScope(std::pmr::memory_resource* pAllocator, ServiceRegistry* pRegistry, IServiceProvider* pRootProvider)
            : m_Table(pAllocator)
            , m_pRegistry(pRegistry)
            , m_pRootProvider(pRootProvider)
        {
            if (pRegistry->m_pRootLifetimeScope == nullptr)
                pRegistry->m_pRootLifetimeScope = this;
        }

        ~LifetimeScope();

        ResultCode Resolve(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);
    };
} // namespace FE::DI
