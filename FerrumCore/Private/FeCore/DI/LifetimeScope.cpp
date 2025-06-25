#include <FeCore/DI/LifetimeScope.h>
#include <FeCore/DI/Registry.h>

namespace FE::DI
{
    ResultCode LifetimeScope::ActivateImpl(const ServiceRegistration registration, Memory::RefCountedObjectBase** result) const
    {
        return m_registry->GetActivator(registration.m_index)->Invoke(m_rootProvider, result);
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

        if (registration.m_lifetime != Lifetime::kTransient)
        {
            // We don't want to hold references to transient objects.
            (*result)->AddRef();
        }

        m_table[registration] = *result;
        m_objectsInActivationOrder.push_back({ *result, registration });
        return ResultCode::kSuccess;
    }


    LifetimeScope::~LifetimeScope()
    {
        for (auto iter = m_objectsInActivationOrder.rbegin(); iter != m_objectsInActivationOrder.rend(); ++iter)
        {
            const auto [object, registration] = *iter;
            if (registration.m_lifetime == Lifetime::kSingleton)
            {
                const uint32_t expectedRefCount = registration.m_isConstant ? 2 : 1;
                FE_Assert(object->GetRefCount() == expectedRefCount);
                object->Release();
            }
        }

        m_table.clear();
        m_objectsInActivationOrder.clear();
    }


    ResultCode LifetimeScope::Resolve(const ServiceRegistration registration, Memory::RefCountedObjectBase** result)
    {
        FE_PROFILER_ZONE();

        switch (registration.m_lifetime)
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
