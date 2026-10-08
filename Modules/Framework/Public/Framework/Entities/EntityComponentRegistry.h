#pragma once
#include <Core/Threading/SpinLock.h>
#include <Framework/Entities/EntityResidencySet.h>
#include <festd/vector.h>
#include <mutex>

namespace FE::Framework
{
    //! @brief Storage policy selected before any instances of the component are created.
    struct ComponentPolicy final
    {
        //! @brief Exclude runtime-only state from authored component records.
        bool m_transient = false;
    };


    //! @brief World-owned stable metadata connecting RTTI operations and optional paired lifecycle hooks.
    struct EntityComponentInfo final
    {
        //! @brief Prepare initialization or activation without blocking.
        using Stage = LifecycleResult (*)(void*, ComponentContext&);
        //! @brief Undo the corresponding initialized or activated state.
        using Undo = void (*)(void*, ComponentContext&);

        //! @brief Reflected component identity or stable RTTI metadata.
        const Rtti::Type* m_type = nullptr;
        //! @brief Component storage or traversal execution policy.
        ComponentPolicy m_policy;

        //! @brief Loading hook that may contribute asset dependencies and remain pending.
        using Load = LifecycleResult (*)(void*, ComponentLoadingContext&);
        //! @brief Undo hook paired with Load.
        using Unload = void (*)(void*, ComponentLoadingContext&);

        //! @brief Optional asynchronous loading adapter.
        Load m_load = nullptr;
        //! @brief Release state acquired by the loading adapter.
        Unload m_unload = nullptr;

        //! @brief Prepare initialized runtime state after required dependencies are ready.
        Stage m_init = nullptr;
        //! @brief Undo initialization in reverse dependency order.
        Undo m_shutdown = nullptr;
        //! @brief Prepare publication; failure leaves the subtree unpublished.
        Stage m_activate = nullptr;
        //! @brief Undo activation before unloading or destruction.
        Undo m_deactivate = nullptr;

        //! @brief Component types that must initialize before this type.
        festd::vector<Rtti::TypeID> m_initAfter;
        //! @brief Transient default-constructed columns added to authored layouts.
        festd::vector<Rtti::TypeID> m_runtimeCompanions;
    };


    // World-owned registration. Generic construction/relocation/serialization always comes from RTTI.
    //! @brief Synchronized world-local registry; relocation is reflected and must not throw.
    struct EntityComponentRegistry final
    {
        //! @brief Release registered metadata after all component storage is destroyed.
        ~EntityComponentRegistry();
        //! @brief Create an empty world-local component registry.
        EntityComponentRegistry() = default;
        EntityComponentRegistry(const EntityComponentRegistry&) = delete;
        EntityComponentRegistry& operator=(const EntityComponentRegistry&) = delete;

        //! @brief Return stable registered metadata or null; registration and lookup are synchronized.
        [[nodiscard]] const EntityComponentInfo* Find(Rtti::TypeID id) const;

        //! @brief Attach a registered transient default-constructible companion to an authored component layout.
        template<class Authored, class Runtime>
        void AddRuntimeCompanion()
        {
            std::lock_guard lock{ m_lock };

            auto* authored = const_cast<EntityComponentInfo*>(FindUnlocked(Rtti::GetTypeID<Authored>()));
            const auto* runtime = FindUnlocked(Rtti::GetTypeID<Runtime>());
            FE_Assert(authored && runtime && runtime->m_policy.m_transient && runtime->m_type->m_defaultConstructor);
            if (festd::find(authored->m_runtimeCompanions, Rtti::GetTypeID<Runtime>()) == authored->m_runtimeCompanions.end())
                authored->m_runtimeCompanions.push_back(Rtti::GetTypeID<Runtime>());
        }

        //! @brief Register reflected relocation and paired lifecycle hooks; repeated registration preserves the original policy.
        template<class T>
        bool Register(festd::span<const Rtti::TypeID> initAfter = {}, ComponentPolicy policy = {})
        {
            std::lock_guard lock{ m_lock };

            const Rtti::Type& type = Rtti::GetType<T>();
            if (FindUnlocked(type.m_id))
                return true;

            FE_Assert(type.m_moveConstructor && type.m_destructor,
                      "Chunk components require RTTI move construction and destruction");

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

        const EntityComponentInfo* FindUnlocked(Rtti::TypeID id) const;
        festd::vector<EntityComponentInfo*> m_entries;
    };
} // namespace FE::Framework
