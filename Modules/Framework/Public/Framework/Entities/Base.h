#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/Base/Hash.h>

namespace FE::Framework
{
    union EntityID;
    struct Entity;
    struct EntitySystem;
    struct EntityComponentInfo;
    struct EntityRegistry;
    struct EntityWorld;
    struct EntityWorldSystem;
    struct EntityLoadingContext;
    struct EntityUpdateContext;

    struct Archetype;
    struct ArchetypeChunk;


    struct ComponentTypeID final : public TypedHandle<ComponentTypeID, uint64_t>
    {
        template<class TComponent>
        static constexpr ComponentTypeID Create()
        {
            return ComponentTypeID{ TypeNameHash<TComponent> };
        }

        template<class TComponent>
        [[nodiscard]] constexpr bool Is() const
        {
            return m_value == TypeNameHash<TComponent>;
        }
    };


    inline constexpr uint32_t kMaxComponentsPerEntity = 512;

    inline constexpr uint32_t kEntityWorldIDBits = 4;
    inline constexpr uint32_t kEntityRegistryIDBits = 28;

    inline constexpr uint32_t kInvalidEntityWorldID = (1 << kEntityWorldIDBits) - 1;
    inline constexpr uint32_t kInvalidEntityRegistryID = (1 << kEntityRegistryIDBits) - 1;
} // namespace FE::Framework


template<>
struct eastl::hash<FE::Framework::ComponentTypeID>
{
    size_t operator()(const FE::Framework::ComponentTypeID id) const
    {
        return id.m_value;
    }
};
