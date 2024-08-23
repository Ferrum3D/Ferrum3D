#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>

namespace FE::DI
{
    ResultCode LifetimeScope::ActivateImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult)
    {
        return m_pRegistry->GetActivator(registration.GetIndex())->Invoke(m_pRootProvider, ppResult);
    }


    ResultCode LifetimeScope::ResolveImpl(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult)
    {
        auto iter = m_Table.find(registration);
        if (iter != m_Table.end())
        {
            *ppResult = iter->second;
            return ResultCode::Success;
        }

        const ResultCode activationResult = ActivateImpl(registration, ppResult);
        if (activationResult != ResultCode::Success)
        {
            *ppResult = nullptr;
            return activationResult;
        }

        if (registration.GetLifetime() != Lifetime::Transient)
        {
            // We don't want to hold references to transient objects.
            (*ppResult)->AddRef();
        }

        m_Table[registration] = *ppResult;
        return ResultCode::Success;
    }


    LifetimeScope::~LifetimeScope()
    {
        for (auto& [registration, pObject] : m_Table)
        {
            pObject->Release();
        }

        m_Table.clear();
    }


    ResultCode LifetimeScope::Resolve(ServiceRegistration registration, Memory::RefCountedObjectBase** ppResult)
    {
        const Lifetime lifetime = registration.GetLifetime();
        switch (lifetime)
        {
        case Lifetime::Transient:
            return ActivateImpl(registration, ppResult);
        case Lifetime::Singleton:
            return ResolveImpl(registration, ppResult);
        case Lifetime::Thread:
        default:
            *ppResult = nullptr;
            return ResultCode::InvalidOperation;
        }
    }
} // namespace FE::DI
