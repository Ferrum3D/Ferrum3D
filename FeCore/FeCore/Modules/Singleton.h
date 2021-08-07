#pragma once
#include <FeCore/Modules/Environment.h>
#include <shared_mutex>

namespace FE
{
    /**
     * @brief This class provides interface to access registered singletons.
     * 
     * This class doesn't own the singleton, memory is managed by the user.
     * 
     * Usage:
     * class IFoo { ... };
     * class Foo : public SingletonImplBase<IFoo> { ... };
     * 
     * Foo* owner = new Foo;
     * IFoo* instance = Singleton<IFoo>::Get();
     * 
     * @tparam T Type of singleton interface.
    */
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
        /**
         * @brief Register the singleton instance.
        */
        inline static void Register(T* instance)
        {
            FE_CORE_ASSERT(instance, "Singleton instance was a nullptr");
            FE_CORE_ASSERT(Get() == nullptr, "Couldn't register singleton twise");
            std::unique_lock lk(m_Mutex);
            m_Instance = Env::CreateGlobalVariableByType<T*>(instance);
        }

        /**
         * @brief Unregister the singleton instance.
        */
        inline static void Unregister()
        {
            FE_CORE_ASSERT(m_Instance, "Singleton instance was a nullptr");
            std::unique_lock lk(m_Mutex);
            *m_Instance = nullptr;
            m_Instance.Reset();
        }

        /**
         * @brief Get the registered singleton instance.
        */
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

    /**
     * @brief Helper class that registers and unregisters derived implementations of T in Singleton<T>.
    */
    template<class T>
    struct SingletonImplBase : public T
    {
        inline SingletonImplBase()
        {
            Singleton<T>::Register(this);
        }

        inline virtual ~SingletonImplBase()
        {
            Singleton<T>::Unregister();
        }
    };
} // namespace FE
