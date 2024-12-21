#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>

namespace FE::DI
{
    ResultCode LifetimeScope::ActivateImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult)
    {
        return m_registry->GetActivator(registration.GetIndex())->Invoke(m_rootProvider, ppResult);
    }


    ResultCode LifetimeScope::ResolveImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult)
    {
        auto iter = m_table.find(registration);
        if (iter != m_table.end())
        {
            *ppResult = iter->second;
            return ResultCode::kSuccess;
        }

        const ResultCode activationResult = ActivateImpl(registration, ppResult);
        if (activationResult != ResultCode::kSuccess)
        {
            *ppResult = nullptr;
            return activationResult;
        }

        if (registration.GetLifetime() != Lifetime::kTransient)
        {
            // We don't want to hold references to transient objects.
            (*ppResult)->AddRef();
        }

        m_table[registration] = *ppResult;
        return ResultCode::kSuccess;
    }


    LifetimeScope::~LifetimeScope()
    {
        for (auto& [registration, pObject] : m_table)
        {
            pObject->Release();
        }

        m_table.clear();
    }


    ResultCode LifetimeScope::Resolve(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult)
    {
        switch (registration.GetLifetime())
        {
        case Lifetime::kTransient:
            return ActivateImpl(registration, ppResult);
        case Lifetime::kSingleton:
            return ResolveImpl(registration, ppResult);
        case Lifetime::kThread:
        default:
            *ppResult = nullptr;
            return ResultCode::kInvalidOperation;
        }
    }
} // namespace FE::DI
