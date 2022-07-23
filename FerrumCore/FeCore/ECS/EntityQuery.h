#pragma once
#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/Entity.h>
#include <FeCore/ECS/EntityArchetype.h>
#include <FeCore/ECS/EntityRegistry.h>
#include <functional>

namespace FE::ECS
{
    //! \brief Entity query allows to iterate over a group of selected entities and their components.
    class EntityQuery : public Object<IObject>
    {
        friend class EntityRegistry;

        EntityRegistry* m_Registry;
        List<EntityArchetype*> m_Archetypes;

        EntityArchetype m_IncludeNone;
        EntityArchetype m_IncludeAll;
        EntityArchetype m_IncludeAny;

        template<class T>
        inline T* GetComponent(ComponentStorage* storage, USize index)
        {
            return storage->GetComponent<T>(index);
        }

    public:
        FE_STRUCT_RTTI(EntityQuery, "BB78DEF5-2D28-4E85-9E78-3CD1E7263B45");

        inline explicit EntityQuery(EntityRegistry* registry)
            : m_Registry(registry)
        {
        }

        template<class... Types>
        inline EntityQuery& NoneOf()
        {
            NoneOf({ ComponentType::Create<Types>()... });
            return *this;
        }

        template<class... Types>
        inline EntityQuery& AllOf()
        {
            AllOf({ ComponentType::Create<Types>()... });
            return *this;
        }

        template<class... Types>
        inline EntityQuery& AnyOf()
        {
            AnyOf({ ComponentType::Create<Types>()... });
            return *this;
        }

        inline void NoneOf(ArraySlice<ComponentType> components)
        {
            m_IncludeNone.m_Layout.Assign(components.begin(), components.end());
        }

        inline void AllOf(ArraySlice<ComponentType> components)
        {
            m_IncludeAll.m_Layout.Assign(components.begin(), components.end());
        }

        inline void AnyOf(ArraySlice<ComponentType> components)
        {
            m_IncludeAny.m_Layout.Assign(components.begin(), components.end());
        }

        inline EntityQuery& Update()
        {
            m_Registry->UpdateEntityQuery(this);
            return *this;
        }

        template<class... Types>
        inline void ForEach(const std::function<void(Types&...)>& function)
        {
            auto types = std::make_tuple(fe_typeid<Types>()...);

            for (auto* archetype : m_Archetypes)
            {
                for (auto* chunk : archetype->m_Chunks)
                {
                    auto storages = std::apply(
                        [chunk](auto... args) {
                            return std::make_tuple(chunk->GetComponentStorage(args)...);
                        },
                        types);

                    for (USize i = 0; i < chunk->Count(); ++i)
                    {
                        auto components = std::apply(
                            [i, this](auto... args) {
                                return std::make_tuple(GetComponent<Types>(args, i)...);
                            },
                            storages);
                        std::apply(
                            [&function](auto... args) {
                                function((*args)...);
                            },
                            components);
                    }
                }
            }
        }
    };
} // namespace FE::ECS
