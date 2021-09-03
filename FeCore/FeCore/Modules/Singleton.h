#pragma once
#include <FeCore/Memory/Object.h>
#include <FeCore/Modules/Environment.h>
#include <shared_mutex>

namespace FE
{
    //! @brief This class provides interface to access registered singletons.
    //!
    //! This class doesn't own the singleton, memory is managed by the user.\n
    //!
    //! Example:
    //! \code{.cpp}
    //!     class IFoo { ... };
    //!     class Foo : public SingletonImplBase<IFoo> { ... };
    //!
    //!     Foo* owner = new Foo;
    //!     IFoo* instance = Singleton<IFoo>::Get();
    //! \endcode
    //!
    //! @tparam T Type of singleton interface.
    template<class T>
    class Singleton
    {
        static Env::GlobalVariable<T*> m_Instance;
        static std::shared_mutex m_Mutex;

        inline static void TryFind()
        {
            std::unique_lock lk(m_Mutex);
            Env::FindGlobalVariableByType<T*>().OnOk([&](const auto&, const auto& variable) {
                m_Instance = variable;
            });
        }

    public:
        FE_CLASS_RTTI(Singleton<T>, "BE31ABA8-37F8-4AE1-8626-9D38FB9D8CB1");

        //! \brief Register the singleton instance.
        //!
        //! The provided instance will be registered in the global environment,
        //! so that it can be used in multiple modules.
        //!
        //! \param [in] instance - The singleton instance to register.
        inline static void Register(T* instance)
        {
            FE_CORE_ASSERT(instance, "Singleton instance was a nullptr");
            FE_CORE_ASSERT(Get() == nullptr, "Couldn't register singleton twice");
            std::unique_lock lk(m_Mutex);
            m_Instance = Env::CreateGlobalVariableByType<T*>(instance);
        }

        //! \brief Unregister the singleton instance.
        inline static void Unregister()
        {
            FE_CORE_ASSERT(m_Instance, "Singleton instance was a nullptr");
            std::unique_lock lk(m_Mutex);
            *m_Instance = nullptr;
            m_Instance.Reset();
        }

        //! \brief Get the registered singleton instance.
        //!
        //! The function will try to find the singleton in all attached modules and cache it in the current module.
        //! If the singleton of type `T` was not yet registered the function will return `nullptr`.
        //!
        //! \return The registered instance.
        inline static T* Get()
        {
            if (!m_Instance)
                TryFind();
            {
                std::shared_lock lk(m_Mutex);
                return m_Instance ? *m_Instance : nullptr;
            }
        }
    };

    template<class T>
    Env::GlobalVariable<T*> Singleton<T>::m_Instance;

    template<class T>
    std::shared_mutex Singleton<T>::m_Mutex;

    //! \brief Helper class that registers and unregisters derived implementations of `T` in \ref Singleton.
    template<class T>
    struct SingletonImplBase : public Object<T>
    {
        FE_CLASS_RTTI(SingletonImplBase, "3C5B1F1F-48B4-4A20-BAFA-70AEE73AC2A3");

        //! \brief Calls \ref Singleton::Register.
        inline SingletonImplBase()
        {
            Singleton<T>::Register(this);
        }

        //! \brief Calls \ref Singleton::Unregister.
        inline virtual ~SingletonImplBase()
        {
            Singleton<T>::Unregister();
        }
    };
} // namespace FE
