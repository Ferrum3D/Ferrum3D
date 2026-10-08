#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    // A transactional view of authored values and hierarchy after preceding commands in the same list.
    //! @brief Transactional hierarchy and authored component view after preceding commands in the same batch.
    struct ReparentContext final
    {
        //! @brief Transaction-local index of the entity being reparented.
        uint32_t m_target;
        //! @brief Transaction-local parent index, or kInvalidIndex to detach.
        uint32_t m_newParent;
        //! @brief Number of entities in the transaction snapshot.
        uint32_t m_entityCount;
        //! @brief Requested local/world transform preservation.
        ReparentMode m_mode;
        //! @brief Borrowed transaction state passed to accessor callbacks.
        void* m_userData;
        //! @brief Read a snapshot parent index through opaque transaction state.
        using ParentLookup = uint32_t (*)(void*, uint32_t);
        //! @brief Read or prepare a writable component through opaque transaction state.
        using ComponentLookup = void* (*)(void*, uint32_t, Rtti::TypeID, bool);

        //! @brief Access the snapshot hierarchy through m_userData.
        ParentLookup m_parent;
        //! @brief Borrow or copy a component through m_userData; the boolean requests write access.
        ComponentLookup m_component;

        //! @brief Read the transaction-local parent index; kInvalidIndex identifies a root.
        uint32_t GetParent(uint32_t entity) const
        {
            return m_parent(m_userData, entity);
        }

        //! @brief Borrow an authored component from the transaction snapshot or return null.
        template<class T>
        const T* Read(uint32_t entity) const
        {
            return static_cast<const T*>(m_component(m_userData, entity, Rtti::GetTypeID<T>(), false));
        }

        //! @brief Prepare a writable transactional copy; values publish only when the entire batch succeeds.
        template<class T>
        T* Write(uint32_t entity) const
        {
            return static_cast<T*>(m_component(m_userData, entity, Rtti::GetTypeID<T>(), true));
        }
    };


    //! @brief Validate and prepare a reparent edit; return false to reject the entire transaction.
    using ReparentHandler = bool (*)(ReparentContext&);
} // namespace FE::Framework
