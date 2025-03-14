#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>

namespace FE::DI
{
    ResultCode LifetimeScope::ActivateImpl(const ServiceRegistration registration, Memory::RefCountedObjectBase** result) const
    {
        return m_registry->GetActivator(registration.GetIndex())->Invoke(m_rootProvider, result);
    }


    ResultCode LifetimeScope::ResolveImpl(const ServiceRegistration registration, Memory::RefCountedObjectBase** result)
    {
        auto iter = m_table.find(registration);
        if (iter != m_table.end())
        {
            *result = iter->second;
            return ResultCode::kSuccess;
        }

        const ResultCode activationResult = ActivateImpl(registration, result);
        if (activationResult != ResultCode::kSuccess)
        {
            *result = nullptr;
            return activationResult;
        }

        if (registration.GetLifetime() != Lifetime::kTransient)
        {
            // We don't want to hold references to transient objects.
            (*result)->AddRef();
        }

        m_table[registration] = *result;
        m_objectsInActivationOrder.push_back(*result);
        return ResultCode::kSuccess;
    }


    LifetimeScope::~LifetimeScope()
    {
        for (auto iter = m_objectsInActivationOrder.rbegin(); iter != m_objectsInActivationOrder.rend(); ++iter)
        {
            auto* object = *iter;
            object->Release();
        }

        m_table.clear();
        m_objectsInActivationOrder.clear();
    }


    ResultCode LifetimeScope::Resolve(const ServiceRegistration registration, Memory::RefCountedObjectBase** result)
    {
        FE_PROFILER_ZONE();

        switch (registration.GetLifetime())
        {
        case Lifetime::kTransient:
            return ActivateImpl(registration, result);
        case Lifetime::kSingleton:
            return ResolveImpl(registration, result);
        case Lifetime::kThread:
        case Lifetime::kCount:
        default:
            *result = nullptr;
            return ResultCode::kInvalidOperation;
        }
    }
} // namespace FE::DI
