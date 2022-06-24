#pragma once
#include <FeCore/Scene/Entity.h>

namespace FE
{
    template<class... Component>
    class EntityIterator
    {
        friend class FeScene;
        entt::registry& m_Registry;

        inline EntityIterator(entt::registry& registry)
            : m_Registry(registry)
        {
        }

    public:
        FE_CLASS_RTTI(EntityIterator, "D7F10571-83B4-4794-93E4-C090E0B00EB8");

        template<class Func>
        inline void ForEach(Func&& func)
        {
            auto view = m_Registry.view<Component...>();
            for (auto e : view)
            {
                Entity entity(m_Registry, e);
                func(entity, view.get<Component>(e)...);
            }
        }
    };

    // \brief Represents a scene in ECS.
    class FeScene
    {
        entt::registry m_Registry;

    public:
        FE_CLASS_RTTI(FeScene, "369F4356-1EB9-4461-84E8-B59D2E8067A1");

        inline Entity CreateEntity()
        {
            return Entity(m_Registry, m_Registry.create());
        }

        template<class... Component>
        inline EntityIterator<Component...> GetIterator()
        {
            return EntityIterator<Component...>(m_Registry);
        }
    };
} // namespace FE
