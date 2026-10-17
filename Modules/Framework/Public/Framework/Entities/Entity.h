#pragma once
#include <Core/Memory/Memory.h>
#include <Framework/Entities/Base.h>

namespace FE::Framework
{
    struct EntityResources;


    //! @brief Stable runtime metadata; component addresses remain valid only until structural commit.
    struct Entity final
    {
        //! @brief Return the generation-checked runtime identity; never serialize this ID.
        [[nodiscard]] EntityID GetID() const
        {
            return m_id;
        }

        //! @brief Return the stable authored or generated identity used by serialized references.
        [[nodiscard]] Uuid GetUuid() const
        {
            return m_uuid;
        }

        [[nodiscard]] Env::Name GetName() const
        {
            return m_name;
        }

        [[nodiscard]] EntityWorld& GetWorld() const;

        [[nodiscard]] EntityRegistry& GetRegistry() const
        {
            return *m_registry;
        }

        [[nodiscard]] Entity* GetParent() const
        {
            return const_cast<Entity*>(m_parent.Get());
        }

        [[nodiscard]] Entity* GetFirstChild() const
        {
            return const_cast<Entity*>(m_firstChild.Get());
        }

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
        bool m_activationPrepared = false;
        uint64_t m_lifecycleEpoch = UINT64_MAX;

        Entity(EntityRegistry& registry, EntityID id, Uuid uuid, Env::Name name);
        ~Entity();
    };
} // namespace FE::Framework
