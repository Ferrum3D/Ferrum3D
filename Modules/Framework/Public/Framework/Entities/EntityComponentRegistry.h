#pragma once
#include <FeCore/Memory/LinearAllocator.h>
#include <Framework/Entities/Base.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    namespace Internal
    {
        template<class TComponent>
        using ComponentInitType = decltype(std::declval<TComponent>().Init());

        template<class TComponent>
        inline constexpr bool kComponentHasInit = festd::detect_v<TComponent, ComponentInitType>;

        template<class TComponent>
        using ComponentShutdownType = decltype(std::declval<TComponent>().Shutdown());

        template<class TComponent>
        inline constexpr bool kComponentHasShutdown = festd::detect_v<TComponent, ComponentShutdownType>;

        template<class TComponent>
        using ComponentLoadType = decltype(std::declval<TComponent>().Load(std::declval<const EntityLoadingContext&>()));

        template<class TComponent>
        inline constexpr bool kComponentHasLoad = festd::detect_v<TComponent, ComponentLoadType>;

        template<class TComponent>
        using ComponentUnloadType = decltype(std::declval<TComponent>().Unload(std::declval<const EntityLoadingContext&>()));

        template<class TComponent>
        inline constexpr bool kComponentHasUnload = festd::detect_v<TComponent, ComponentUnloadType>;
    } // namespace Internal


    using ConstructComponentFunction = void (*)(void* component);
    using MoveConstructComponentFunction = void (*)(void* destination, void* source);
    using DestroyComponentFunction = void (*)(void* component);
    using InitComponentFunction = void (*)(void* component);
    using ShutdownComponentFunction = void (*)(void* component);
    using LoadComponentFunction = void (*)(void* component, const EntityLoadingContext& context);
    using UnloadComponentFunction = void (*)(void* component, const EntityLoadingContext& context);


    struct EntityComponentInfo final
    {
        ComponentTypeID m_typeID;
        uint32_t m_byteSize = 0;
        uint32_t m_byteAlignment = 0;
        ConstructComponentFunction m_construct = nullptr;
        MoveConstructComponentFunction m_moveConstruct = nullptr;
        DestroyComponentFunction m_destroy = nullptr;
        InitComponentFunction m_init = nullptr;
        ShutdownComponentFunction m_shutdown = nullptr;
        LoadComponentFunction m_load = nullptr;
        UnloadComponentFunction m_unload = nullptr;
    };


    struct EntityComponentRegistry final
    {
        template<class TComponent>
        ComponentTypeID RegisterComponent()
        {
            const ComponentTypeID typeID = ComponentTypeID::Create<TComponent>();
            if (m_entries.find(typeID) != m_entries.end())
                return typeID;

            EntityComponentInfo* entry = Memory::New<EntityComponentInfo>(&m_infoAllocator);
            entry->m_typeID = typeID;
            entry->m_byteSize = sizeof(TComponent);
            entry->m_byteAlignment = alignof(TComponent);

            entry->m_construct = [](void* component) {
                new (component) TComponent();
            };
            entry->m_moveConstruct = [](void* destination, void* source) {
                new (destination) TComponent(std::move(*static_cast<TComponent*>(source)));
            };
            entry->m_destroy = [](void* component) {
                static_cast<TComponent*>(component)->~TComponent();
            };

            if constexpr (Internal::kComponentHasInit<TComponent>)
            {
                entry->m_init = [](void* component) {
                    static_cast<TComponent*>(component)->Init();
                };

                static_assert(Internal::kComponentHasShutdown<TComponent>);
            }

            if constexpr (Internal::kComponentHasShutdown<TComponent>)
            {
                entry->m_shutdown = [](void* component) {
                    static_cast<TComponent*>(component)->Shutdown();
                };

                static_assert(Internal::kComponentHasInit<TComponent>);
            }

            if constexpr (Internal::kComponentHasLoad<TComponent>)
            {
                entry->m_load = [](void* component, const EntityLoadingContext& context) {
                    static_cast<TComponent*>(component)->Load(context);
                };

                static_assert(Internal::kComponentHasUnload<TComponent>);
            }

            if constexpr (Internal::kComponentHasUnload<TComponent>)
            {
                entry->m_unload = [](void* component, const EntityLoadingContext& context) {
                    static_cast<TComponent*>(component)->Unload(context);
                };

                static_assert(Internal::kComponentHasLoad<TComponent>);
            }

            RegisterEntry(entry);
            return typeID;
        }

        [[nodiscard]] const EntityComponentInfo* GetComponentInfo(ComponentTypeID typeID) const;

        static EntityComponentRegistry& Get();

    private:
        void RegisterEntry(const EntityComponentInfo* entry);

        mutable Threading::SpinLock m_lock;
        festd::segmented_unordered_dense_map<ComponentTypeID, const EntityComponentInfo*> m_entries;
        Memory::LinearAllocator m_infoAllocator;
    };
} // namespace FE::Framework
