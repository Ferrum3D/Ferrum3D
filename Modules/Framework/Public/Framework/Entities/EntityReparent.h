#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    // A transactional view of authored values and hierarchy after preceding commands in the same list.
    struct ReparentContext final
    {
        uint32_t m_target;
        uint32_t m_newParent;
        uint32_t m_entityCount;
        ReparentMode m_mode;
        void* m_userData;
        uint32_t (*m_parent)(void*, uint32_t);
        void* (*m_component)(void*, uint32_t, Rtti::TypeID, bool);

        uint32_t GetParent(uint32_t entity) const
        {
            return m_parent(m_userData, entity);
        }

        template<class T>
        const T* Read(uint32_t entity) const
        {
            return static_cast<const T*>(m_component(m_userData, entity, Rtti::GetTypeID<T>(), false));
        }

        template<class T>
        T* Write(uint32_t entity) const
        {
            return static_cast<T*>(m_component(m_userData, entity, Rtti::GetTypeID<T>(), true));
        }
    };


    using ReparentHandler = bool (*)(ReparentContext&);
} // namespace FE::Framework
