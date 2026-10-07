#pragma once
#include <Core/Threading/SpinLock.h>
#include <Framework/Entities/EntityResidencySet.h>
#include <festd/vector.h>
#include <mutex>

namespace FE::Framework
{
    struct ComponentPolicy final
    {
        bool m_transient = false;
    };


    struct EntityComponentInfo final
    {
        const Rtti::Type* m_type = nullptr;
        uint32_t m_index = 0;
        ComponentPolicy m_policy;
        using Stage = LifecycleResult (*)(void*, ComponentContext&);
        using Undo = void (*)(void*, ComponentContext&);
        LifecycleResult (*m_load)(void*, ComponentLoadingContext&) = nullptr;
        void (*m_unload)(void*, ComponentLoadingContext&) = nullptr;
        Stage m_init = nullptr;
        Undo m_shutdown = nullptr;
        Stage m_activate = nullptr;
        Undo m_deactivate = nullptr;
        festd::vector<Rtti::TypeID> m_initAfter;
        festd::vector<Rtti::TypeID> m_runtimeCompanions;
    };


    // World-owned registration. Generic construction/relocation/serialization always comes from RTTI.
    struct EntityComponentRegistry final
    {
        ~EntityComponentRegistry();
        EntityComponentRegistry() = default;
        EntityComponentRegistry(const EntityComponentRegistry&) = delete;
        EntityComponentRegistry& operator=(const EntityComponentRegistry&) = delete;
        [[nodiscard]] const EntityComponentInfo* Find(Rtti::TypeID id) const;
        [[nodiscard]] festd::ascii_view GetLastError() const
        {
            std::lock_guard lock{ m_lock };
            return m_lastError;
        }

        template<class Authored, class Runtime>
        void AddRuntimeCompanion()
        {
            std::lock_guard lock{ m_lock };
            auto* authored = const_cast<EntityComponentInfo*>(FindUnlocked(Rtti::GetTypeID<Authored>()));
            FE_Assert(authored && FindUnlocked(Rtti::GetTypeID<Runtime>()));
            if (festd::find(authored->m_runtimeCompanions, Rtti::GetTypeID<Runtime>()) == authored->m_runtimeCompanions.end())
                authored->m_runtimeCompanions.push_back(Rtti::GetTypeID<Runtime>());
        }

        template<class T>
        bool Register(festd::span<const Rtti::TypeID> initAfter = {}, ComponentPolicy policy = {})
        {
            std::lock_guard lock{ m_lock };
            m_lastError = {};

            const Rtti::Type& type = Rtti::GetType<T>();
            if (FindUnlocked(type.m_id))
                return true;

            if (!type.m_moveConstructor || !type.m_noThrowMove || !type.m_destructor)
            {
                m_lastError = "Chunk components require RTTI move construction, a no-throw move, and destruction";
                return false;
            }

            constexpr bool hasLoad = requires(T& component, ComponentLoadingContext& context) {
                { component.Load(context) } -> std::same_as<LifecycleResult>;
            };
            constexpr bool hasUnload = requires(T& component, ComponentLoadingContext& context) {
                { component.Unload(context) } -> std::same_as<void>;
            };
            constexpr bool hasInit = requires(T& component, ComponentContext& context) {
                { component.Init(context) } -> std::same_as<LifecycleResult>;
            };
            constexpr bool hasShutdown = requires(T& component, ComponentContext& context) {
                { component.Shutdown(context) } -> std::same_as<void>;
            };
            constexpr bool hasActivate = requires(T& component, ComponentContext& context) {
                { component.Activate(context) } -> std::same_as<LifecycleResult>;
            };
            constexpr bool hasDeactivate = requires(T& component, ComponentContext& context) {
                { component.Deactivate(context) } -> std::same_as<void>;
            };

            static_assert(hasLoad == hasUnload, "Load/Unload must be paired and use ComponentLoadingContext");
            static_assert(hasInit == hasShutdown, "Init/Shutdown must be paired and use ComponentContext");
            static_assert(hasActivate == hasDeactivate, "Activate/Deactivate must be paired and use ComponentContext");
            // A named but unsupported hook is an error, rather than a silently ignored lifecycle method.
            static_assert(!requires { &T::Load; } || hasLoad, "Load must return LifecycleResult");
            static_assert(!requires { &T::Unload; } || hasUnload, "Unload must return void");
            static_assert(!requires { &T::Init; } || hasInit, "Init must return LifecycleResult");
            static_assert(!requires { &T::Shutdown; } || hasShutdown, "Shutdown must return void");
            static_assert(!requires { &T::Activate; } || hasActivate, "Activate must return LifecycleResult");
            static_assert(!requires { &T::Deactivate; } || hasDeactivate, "Deactivate must return void");

            auto* entry = Memory::DefaultNew<EntityComponentInfo>();
            entry->m_type = &type;
            entry->m_policy = policy;
            entry->m_index = m_entries.size();
            entry->m_initAfter.assign(initAfter.begin(), initAfter.end());

            if constexpr (hasLoad)
            {
                entry->m_load = [](void* component, ComponentLoadingContext& context) {
                    return static_cast<T*>(component)->Load(context);
                };
                entry->m_unload = [](void* component, ComponentLoadingContext& context) {
                    static_cast<T*>(component)->Unload(context);
                };
            }

            if constexpr (hasInit)
            {
                entry->m_init = [](void* component, ComponentContext& context) {
                    return static_cast<T*>(component)->Init(context);
                };
                entry->m_shutdown = [](void* component, ComponentContext& context) {
                    static_cast<T*>(component)->Shutdown(context);
                };
            }

            if constexpr (hasActivate)
            {
                entry->m_activate = [](void* component, ComponentContext& context) {
                    return static_cast<T*>(component)->Activate(context);
                };
                entry->m_deactivate = [](void* component, ComponentContext& context) {
                    static_cast<T*>(component)->Deactivate(context);
                };
            }

            m_entries.push_back(entry);
            return true;
        }

    private:
        mutable Threading::SpinLock m_lock;
        festd::ascii_view m_lastError;
        const EntityComponentInfo* FindUnlocked(Rtti::TypeID id) const;
        festd::vector<EntityComponentInfo*> m_entries;
    };
} // namespace FE::Framework
