#pragma once
#include "FeEntity.h"

namespace FE
{
    template<class... Component>
    class FeEntityIterator
    {
        friend class FeScene;
        entt::registry& m_Registry;

        inline FeEntityIterator(entt::registry& registry)
            : m_Registry(registry)
        {
        }

    public:
        template<class Func>
        inline void ForEach(Func&& func)
        {
            auto view = m_Registry.view<Component...>();
            for (auto e : view)
            {
                FeEntity entity{ m_Registry, e };
                func(entity, view.get<Component>(e)...);
            }
        }
    };

    class FeScene
    {
        entt::registry m_Registry;

    public:
        inline FeEntity CreateEntity()
        {
            return FeEntity{ m_Registry, m_Registry.create() };
        }

        template<class... Component>
        inline FeEntityIterator<Component...> GetIterator()
        {
            return FeEntityIterator<Component...>(m_Registry);
        }
    };
} // namespace FE
