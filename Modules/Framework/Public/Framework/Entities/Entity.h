#pragma once
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    // Stable until destruction; component addresses are borrowed only until the next structural commit.
    // Metadata is read-only during traversal. All structural edits go through EntityCommandList.
    struct Entity final
    {
        [[nodiscard]] EntityID GetID() const
        {
            return m_id;
        }


        [[nodiscard]] Uuid GetUuid() const
        {
            return m_uuid;
        }


        [[nodiscard]] Env::Name GetName() const
        {
            return m_name;
        }


        [[nodiscard]] EntityWorld& GetWorld() const
        {
            return *m_world;
        }


        [[nodiscard]] EntityRegistry& GetRegistry() const
        {
            return *m_registry;
        }


        [[nodiscard]] Entity* GetParent() const
        {
            return m_parent;
        }


        [[nodiscard]] Entity* GetFirstChild() const
        {
            return m_firstChild;
        }


        [[nodiscard]] Entity* GetNextSibling() const
        {
            return m_nextSibling;
        }


        [[nodiscard]] bool IsActive() const
        {
            return m_active;
        }


        [[nodiscard]] bool HasFailed() const
        {
            return m_failed;
        }


        [[nodiscard]] void* FindComponent(Rtti::TypeID type, bool write = false) const;
        template<class T>
        [[nodiscard]] T* FindComponent() const
        {
            return static_cast<T*>(FindComponent(Rtti::GetTypeID<std::remove_const_t<T>>(), !std::is_const_v<T>));
        }

    private:
        friend EntityWorld;
        friend ArchetypeChunk;
        struct Runtime;
        EntityWorld* m_world = nullptr;
        EntityRegistry* m_registry = nullptr;
        const EntityID m_id;
        const Uuid m_uuid;
        Env::Name m_name;
        Entity* m_parent = nullptr;
        Entity* m_firstChild = nullptr;
        Entity* m_lastChild = nullptr;
        Entity* m_previousSibling = nullptr;
        Entity* m_nextSibling = nullptr;
        ArchetypeChunk* m_chunk = nullptr;
        uint32_t m_row = 0;
        bool m_active = false;
        bool m_wantsActive = true;
        bool m_failed = false;
        Runtime* m_runtime = nullptr;
        Entity(EntityWorld& world, EntityRegistry& registry, EntityID id, Uuid uuid, Env::Name name);
        ~Entity();
    };
} // namespace FE::Framework
