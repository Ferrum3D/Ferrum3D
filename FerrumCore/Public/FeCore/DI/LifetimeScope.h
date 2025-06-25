#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/DI/Registration.h>
#include <FeCore/DI/Registry.h>
#include <festd/unordered_map.h>

namespace FE::DI
{
    struct LifetimeScope final
    {
        LifetimeScope(std::pmr::memory_resource* allocator, ServiceRegistry* registry, IServiceProvider* pRootProvider)
            : m_table(allocator)
            , m_objectsInActivationOrder(allocator)
            , m_registry(registry)
            , m_rootProvider(pRootProvider)
        {
            if (registry->m_rootLifetimeScope == nullptr)
                registry->m_rootLifetimeScope = this;
        }

        ~LifetimeScope();

        ResultCode Resolve(ServiceRegistration registration, Memory::RefCountedObjectBase** result);

    private:
        struct Entry final
        {
            Memory::RefCountedObjectBase* m_object;
            ServiceRegistration m_registration;
        };

        festd::pmr::segmented_unordered_dense_map<ServiceRegistration, Memory::RefCountedObjectBase*> m_table;
        SegmentedVector<Entry> m_objectsInActivationOrder;

        ServiceRegistry* m_registry;
        IServiceProvider* m_rootProvider;

        ResultCode ActivateImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult) const;
        ResultCode ResolveImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult);
    };
} // namespace FE::DI
