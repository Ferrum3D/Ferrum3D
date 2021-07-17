#pragma once
#include <entt/entt.hpp>

namespace FE
{
    class FeEntity
    {
        friend class FeScene;

        entt::registry& m_Registry;
        entt::entity m_Entity;

    public:
        inline FeEntity(entt::registry& registry, entt::entity entity)
            : m_Registry(registry)
            , m_Entity(entity)
        {
        }

        template<class T, class... Args>
        T& AddComponent(Args&&... args)
        {
            return m_Registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
        }

        template<class T>
        T& GetComponent()
        {
            return m_Registry.get<T>(m_Entity);
        }

        template<class T>
        const T& GetComponent() const
        {
            return m_Registry.get<T>(m_Entity);
        }

        void Destroy()
        {
            m_Registry.destroy(m_Entity);
        }

        bool IsValid()
        {
            return m_Registry.valid(m_Entity);
        }
    };
} // namespace FE
