#pragma once
#include <FeCore/Modules/Environment.h>
#include <FeCore/Threading/SharedSpinLock.h>

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
    class ServiceLocator final
    {
        static T* m_instance;
        static Env::GlobalVariable<T*> m_owner;
        static Threading::SharedSpinLock m_mutex;

        static void TryFind()
        {
            std::unique_lock lk{ m_mutex };
            const Env::GlobalVariable variable = Env::FindGlobalVariableByType<T*>();
            if (variable)
            {
                m_instance = *variable;
            }
        }

    public:
        FE_RTTI_Class(ServiceLocator, "BE31ABA8-37F8-4AE1-8626-9D38FB9D8CB1");

        //! @brief Register the instance.
        //!
        //! The provided instance will be registered in the global environment,
        //! so that it can be used in multiple modules.
        //!
        //! @param instance The instance to register.
        static void Register(T* instance)
        {
            FE_Assert(instance, "ServiceLocator instance was a nullptr");
            FE_Assert(Get() == nullptr, "Couldn't register a ServiceLocator instance twice");
            std::unique_lock lk{ m_mutex };
            m_owner = Env::CreateGlobalVariableByType<T*>(instance);
        }

        //! @brief Unregister the instance.
        static void Unregister()
        {
            FE_Assert(m_owner, "ServiceLocator instance was a nullptr");
            std::unique_lock lk{ m_mutex };
            *m_owner = nullptr;
            m_owner.Reset();
        }

        //! @brief Get the registered instance.
        //!
        //! The function will try to find the instance in all attached modules and cache it in the current module.
        //! If the instance of type `T` was not yet registered the function will return `nullptr`.
        //!
        //! @return The registered instance.
        static T* Get()
        {
            if (!m_instance)
            {
                TryFind();
            }
            {
                std::shared_lock lk{ m_mutex };
                return m_instance;
            }
        }
    };


    template<class T>
    Env::GlobalVariable<T*> ServiceLocator<T>::m_owner;


    template<class T>
    T* ServiceLocator<T>::m_instance;


    template<class T>
    Threading::SharedSpinLock ServiceLocator<T>::m_mutex;


    struct ServiceLocatorObjectMarker
    {
    };


    //! @brief Helper class that registers and unregisters instance in ServiceLocator.
    //!
    //! @tparam TBase       The base class to derive.
    //! @tparam TInterface  The interface to register in ServiceLocator.
    template<class TBase, class TInterface = TBase>
    struct ServiceLocatorImplBase
        : public TBase
        , public ServiceLocatorObjectMarker
    {
        FE_RTTI_Class(ServiceLocatorImplBase, "3C5B1F1F-48B4-4A20-BAFA-70AEE73AC2A3");

        //! @brief Calls ServiceLocator::Register.
        ServiceLocatorImplBase()
        {
            ServiceLocator<TInterface>::Register(static_cast<TInterface*>(this));
        }

        //! @brief Calls ServiceLocator::Unregister.
        virtual ~ServiceLocatorImplBase()
        {
            ServiceLocator<TInterface>::Unregister();
        }

        ServiceLocatorImplBase(const ServiceLocatorImplBase&) = delete;
        ServiceLocatorImplBase(ServiceLocatorImplBase&&) = delete;
        ServiceLocatorImplBase& operator=(const ServiceLocatorImplBase&) = delete;
        ServiceLocatorImplBase& operator=(ServiceLocatorImplBase&&) = delete;
    };
} // namespace FE
