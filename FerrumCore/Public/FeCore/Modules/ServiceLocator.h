﻿#pragma once
#include <FeCore/Modules/Environment.h>
#include <shared_mutex>

namespace FE
{
    //! @brief This class provides interface to access registered instance of a class.
    //!
    //! Doesn't own the registered class, memory is managed by the user.
    //! The instance can be shared between different modules and is created and destroyed explicitly.
    //!
    //! Example:
    //! \code{.cpp}
    //!     class IFoo { ... };
    //!     class Foo : public ServiceLocatorImplBase<IFoo> { ... };
    //!
    //!     Foo* owner = new Foo;
    //!     IFoo* instance = ServiceLocator<IFoo>::Get();
    //! \endcode
    //!
    //! @tparam T Type of the interface.
    template<class T>
    class ServiceLocator
    {
        static T* m_Instance;
        static Env::GlobalVariable<T*> m_Owner;
        static std::shared_mutex m_Mutex;

        static void TryFind()
        {
            std::unique_lock lk(m_Mutex);
            const Env::GlobalVariable variable = Env::FindGlobalVariableByType<T*>();
            if (variable)
            {
                m_Instance = *variable;
            }
        }

    public:
        FE_RTTI_Class(ServiceLocator<T>, "BE31ABA8-37F8-4AE1-8626-9D38FB9D8CB1");

        //! @brief Register the instance.
        //!
        //! The provided instance will be registered in the global environment,
        //! so that it can be used in multiple modules.
        //!
        //! @param instance - The instance to register.
        static void Register(T* instance)
        {
            FE_CORE_ASSERT(instance, "ServiceLocator instance was a nullptr");
            FE_CORE_ASSERT(Get() == nullptr, "Couldn't register a ServiceLocator instance twice");
            std::unique_lock lk(m_Mutex);
            m_Owner = Env::CreateGlobalVariableByType<T*>(instance);
        }

        //! @brief Unregister the instance.
        static void Unregister()
        {
            FE_CORE_ASSERT(m_Owner, "ServiceLocator instance was a nullptr");
            std::unique_lock lk(m_Mutex);
            *m_Owner = nullptr;
            m_Owner.Reset();
        }

        //! @brief Get the registered instance.
        //!
        //! The function will try to find the instance in all attached modules and cache it in the current module.
        //! If the instance of type `T` was not yet registered the function will return `nullptr`.
        //!
        //! @return The registered instance.
        static T* Get()
        {
            if (!m_Instance)
            {
                TryFind();
            }
            {
                std::shared_lock lk(m_Mutex);
                return m_Instance;
            }
        }
    };

    template<class T>
    Env::GlobalVariable<T*> ServiceLocator<T>::m_Owner;

    template<class T>
    T* ServiceLocator<T>::m_Instance;

    template<class T>
    std::shared_mutex ServiceLocator<T>::m_Mutex;

    struct ServiceLocatorObjectMarker
    {
    };

    //! @brief Helper class that registers and unregisters instance in ServiceLocator.
    //!
    //! @tparam TBase - The base class to derive.
    //! @tparam TInterface - The interface to register in ServiceLocator.
    template<class TBase, class TInterface = TBase>
    struct ServiceLocatorImplBase
        : public TBase
        , public ServiceLocatorObjectMarker
    {
        FE_RTTI_Class(ServiceLocatorImplBase, "3C5B1F1F-48B4-4A20-BAFA-70AEE73AC2A3");

        //! @brief Calls \ref ServiceLocator::Register.
        ServiceLocatorImplBase()
        {
            ServiceLocator<TInterface>::Register(static_cast<TInterface*>(this));
        }

        //! @brief Calls \ref ServiceLocator::Unregister.
        virtual ~ServiceLocatorImplBase()
        {
            ServiceLocator<TInterface>::Unregister();
        }
    };
} // namespace FE
