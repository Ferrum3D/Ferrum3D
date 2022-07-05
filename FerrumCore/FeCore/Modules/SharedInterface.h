#pragma once
#include <FeCore/Memory/Object.h>
#include <FeCore/Modules/Environment.h>
#include <shared_mutex>

namespace FE
{
    //! \brief This class provides interface to access registered instance of a class.
    //!
    //! This class doesn't own the registered class, memory is managed by the user.
    //! It is similar to the singleton pattern, but the instance can be shared between
    //! different modules and is created and destroyed explicitly.
    //!
    //! Example:
    //! \code{.cpp}
    //!     class IFoo { ... };
    //!     class Foo : public SharedInterfaceImplBase<IFoo> { ... };
    //!
    //!     Foo* owner = new Foo;
    //!     IFoo* instance = SharedInterface<IFoo>::Get();
    //! \endcode
    //!
    //! \tparam T Type of the interface.
    template<class T>
    class SharedInterface
    {
        static T* m_Instance;
        static Env::GlobalVariable<T*> m_Owner;
        static std::shared_mutex m_Mutex;

        inline static void TryFind()
        {
            std::unique_lock lk(m_Mutex);
            Env::FindGlobalVariableByType<T*>().OnOk([&](const auto&, const auto& variable) {
                m_Instance = *variable;
            });
        }

    public:
        FE_CLASS_RTTI(SharedInterface<T>, "BE31ABA8-37F8-4AE1-8626-9D38FB9D8CB1");

        //! \brief Register the instance.
        //!
        //! The provided instance will be registered in the global environment,
        //! so that it can be used in multiple modules.
        //!
        //! \param [in] instance - The instance to register.
        inline static void Register(T* instance)
        {
            FE_CORE_ASSERT(instance, "SharedInterface instance was a nullptr");
            FE_CORE_ASSERT(Get() == nullptr, "Couldn't register a SharedInterface instance twice");
            std::unique_lock lk(m_Mutex);
            m_Owner = Env::CreateGlobalVariableByType<T*>(instance);
        }

        //! \brief Unregister the instance.
        inline static void Unregister()
        {
            FE_CORE_ASSERT(m_Owner, "SharedInterface instance was a nullptr");
            std::unique_lock lk(m_Mutex);
            *m_Owner = nullptr;
            m_Owner.Reset();
        }

        //! \brief Get the registered instance.
        //!
        //! The function will try to find the instance in all attached modules and cache it in the current module.
        //! If the instance of type `T` was not yet registered the function will return `nullptr`.
        //!
        //! \return The registered instance.
        inline static T* Get()
        {
            if (!m_Instance)
                TryFind();
            {
                std::shared_lock lk(m_Mutex);
                return m_Instance;
            }
        }
    };

    template<class T>
    Env::GlobalVariable<T*> SharedInterface<T>::m_Owner;

    template<class T>
    T* SharedInterface<T>::m_Instance;

    template<class T>
    std::shared_mutex SharedInterface<T>::m_Mutex;

    //! \brief Helper class that registers and unregisters instance in SharedInterface.
    //!
    //! \tparam TBase - The base class to derive.
    //! \tparam TInterface - The interface to register in SharedInterface.
    template<class TBase, class TInterface = TBase>
    struct SharedInterfaceImplBase : public Object<TBase>
    {
        FE_CLASS_RTTI(SharedInterfaceImplBase, "3C5B1F1F-48B4-4A20-BAFA-70AEE73AC2A3");

        //! \brief Calls \ref SharedInterface::Register.
        inline SharedInterfaceImplBase()
        {
            SharedInterface<TInterface>::Register(static_cast<TInterface*>(this));
        }

        //! \brief Calls \ref SharedInterface::Unregister.
        inline virtual ~SharedInterfaceImplBase()
        {
            SharedInterface<TInterface>::Unregister();
        }
    };
} // namespace FE
