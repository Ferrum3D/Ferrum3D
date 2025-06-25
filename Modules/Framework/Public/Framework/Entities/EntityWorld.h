#pragma once
#include <Framework/Entities/Base.h>
#include <Framework/Entities/EntityUpdateContext.h>
#include <festd/bit_vector.h>
#include <festd/intrusive_list.h>
#include <festd/vector.h>

namespace FE::Framework
{
    struct EntityWorld final : public festd::intrusive_list_node
    {
        struct [[nodiscard]] ListReader final
        {
            ~ListReader();

            ListReader(const ListReader&) = delete;
            ListReader& operator=(const ListReader&) = delete;
            ListReader(ListReader&& other) noexcept;
            ListReader& operator=(ListReader&& other) noexcept;

            [[nodiscard]] festd::intrusive_list<EntityWorld>::iterator begin() const
            {
                return m_list->begin();
            }

            [[nodiscard]] festd::intrusive_list<EntityWorld>::iterator end() const
            {
                return m_list->end();
            }

        private:
            friend EntityWorld;

            explicit ListReader(festd::intrusive_list<EntityWorld>* list);

            festd::intrusive_list<EntityWorld>* m_list;
        };

        FE_RTTI_Class(EntityWorld, "069E3B9C-CC10-4989-A6DB-37095DFDB990");

        EntityWorld();
        ~EntityWorld();

        EntityWorld(const EntityWorld&) = delete;
        EntityWorld& operator=(const EntityWorld&) = delete;
        EntityWorld(EntityWorld&&) = delete;
        EntityWorld& operator=(EntityWorld&&) = delete;

        static ListReader GetWorldList();

        [[nodiscard]] uint32_t GetID() const
        {
            return m_ID;
        }

        [[nodiscard]] EntityRegistry* CreateRegistry();

        [[nodiscard]] EntityRegistry* GetPersistentRegistry() const
        {
            return m_registries[0];
        }

        void AddSystem(EntityWorldSystem* system);
        void RemoveSystem(EntityWorldSystem* system);

        void UpdateLoadingState();
        void Update();

    private:
        friend EntityRegistry;

        uint32_t m_ID = kInvalidEntityWorldID;

        Threading::SpinLock m_lock;
        festd::vector<EntityWorldSystem*> m_worldSystems;

        festd::inline_vector<EntityRegistry*> m_registries;
        festd::inline_vector<uint32_t> m_registryIDs;
        festd::bit_vector m_freeRegistryIDs;

        std::atomic<bool> m_isUpdating = false;
        EntityLoadingContext m_loadingContext;
        EntityUpdateContext m_updateContext;
    };
} // namespace FE::Framework
