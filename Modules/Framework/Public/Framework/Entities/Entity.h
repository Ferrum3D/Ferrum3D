#pragma once
#include <Core/Memory/Memory.h>
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    struct EntityResources;


    // Stable until destruction; component addresses are borrowed only until the next structural commit.
    // Metadata is read-only during traversal. All structural edits go through EntityCommandList.
    //! @brief Stable runtime metadata; component addresses remain valid only until structural commit.
    struct Entity final
    {
        //! @brief Return the generation-checked runtime identity; never serialize this ID.
        [[nodiscard]] EntityID GetID() const
        {
            //! @brief Stable identity key.
            return m_id;
        }

        //! @brief Return the stable authored or generated identity used by serialized references.
        [[nodiscard]] Uuid GetUuid() const
        {
            //! @brief Authored or referenced UUID; never a runtime entity handle.
            return m_uuid;
        }

        //! @brief Return the interned runtime display name.
        [[nodiscard]] Env::Name GetName() const
        {
            //! @brief Display name; not an identity key.
            return m_name;
        }

        //! @brief Return the owning world.
        [[nodiscard]] EntityWorld& GetWorld() const;

        //! @brief Return the owning residency group.
        [[nodiscard]] EntityRegistry& GetRegistry() const
        {
            //! @brief Borrowed residency owner shared by rows in this chunk.
            return *m_registry;
        }

        //! @brief Return the immediate parent or null for a root.
        [[nodiscard]] Entity* GetParent() const
        {
            return const_cast<Entity*>(m_parent.Get());
        }

        //! @brief Return the first child in sibling order, or null.
        [[nodiscard]] Entity* GetFirstChild() const
        {
            return const_cast<Entity*>(m_firstChild.Get());
        }

        //! @brief Return the next sibling, or null.
        [[nodiscard]] Entity* GetNextSibling() const
        {
            return const_cast<Entity*>(m_nextSibling.Get());
        }

        //! @brief Return whether the entity is published to queries.
        [[nodiscard]] bool IsActive() const
        {
            return m_active;
        }

        //! @brief Return whether component preparation failed; allocation may still exist.
        [[nodiscard]] bool HasFailed() const
        {
            return m_failed;
        }

        //! @brief Borrow the component until the next structural commit, or return null; mutable lookups require write access.
        [[nodiscard]] void* FindComponent(Rtti::TypeID type, bool write = false) const;

        //! @brief Borrow the component until the next structural commit, or return null; mutable lookups require write access.
        template<class T>
        [[nodiscard]] T* FindComponent() const
        {
            return static_cast<T*>(FindComponent(Rtti::GetTypeID<std::remove_const_t<T>>(), !std::is_const_v<T>));
        }

    private:
        friend EntityWorld;
        friend struct EntityScheduler;
        friend ArchetypeChunk;

        const Uuid m_uuid;
        const EntityID m_id;

        Env::Name m_name;
        uint32_t m_row = 0;

        EntityRegistry* m_registry;
        ArchetypeChunk* m_chunk = nullptr;
        EntityResources* m_resources = nullptr;

        Memory::ShortPtr<Entity> m_parent{ nullptr };
        Memory::ShortPtr<Entity> m_firstChild{ nullptr };
        Memory::ShortPtr<Entity> m_lastChild{ nullptr };
        Memory::ShortPtr<Entity> m_previousSibling{ nullptr };
        Memory::ShortPtr<Entity> m_nextSibling{ nullptr };

        ResidencyScope m_residencyScope = ResidencyScope::kEntity;
        bool m_active = false;
        bool m_wantsActive = true;
        bool m_failed = false;
        bool m_prepared = false;

        Entity(EntityRegistry& registry, EntityID id, Uuid uuid, Env::Name name);
        ~Entity();
    };
} // namespace FE::Framework
