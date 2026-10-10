#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    //! @brief List-local creation token; it cannot be used by a different recorder.
    struct EntityToken final
    {
        //! @brief Globally reserved recorder identity; zero is invalid and tokens cannot cross command lists.
        uint16_t m_list = 0;
        //! @brief List-local creation index; UINT16_MAX is invalid.
        uint16_t m_index = UINT16_MAX;
    };
    static_assert(sizeof(EntityToken) == 4);


    //! @brief Command target containing either a runtime ID or a list-local creation token.
    struct EntityTarget final
    {
        //! @brief Stable identity key.
        EntityID m_id;
        //! @brief List-local target identity when no runtime ID is present.
        EntityToken m_token;

        EntityTarget() = default;

        EntityTarget(const EntityID id)
            : m_id(id)
        {
        }

        EntityTarget(const EntityToken token)
            : m_token(token)
        {
        }
    };


    //! @brief Single-recorder transactional batch; independent conflicting batches are rejected.
    //!
    //! One recorder owns a list. Submit transfers ownership; tokens cannot refer to a different list.
    //! Within a list rename, parent and component replacement use the final recorded value.
    //! Destroy discards later operations on its target. Independent conflicting lists are rejected.
    struct EntityCommandList final
    {
        explicit EntityCommandList(EntityWorld& world);
        ~EntityCommandList();

        EntityCommandList(EntityCommandList&&) noexcept;
        EntityCommandList& operator=(EntityCommandList&&) noexcept;
        EntityCommandList(const EntityCommandList&) = delete;
        EntityCommandList& operator=(const EntityCommandList&) = delete;

        //! @brief Record an inactive entity creation, up to 65,535 per list; a null UUID generates a random identity.
        EntityToken CreateEntity(EntityRegistry& registry, Env::Name name = {}, Uuid uuid = Uuid::kNull,
                                 ResidencyScope residency = ResidencyScope::kEntity);

        //! @brief Destroy the target and descendants; later operations on the destroyed target are discarded.
        void Destroy(EntityTarget target);

        void Rename(EntityTarget target, Env::Name name);

        //! @brief Record final hierarchy and local/world preservation; an empty parent detaches the target.
        void SetParent(EntityTarget target, EntityTarget parent = {}, ReparentMode mode = ReparentMode::kPreserveWorld);

        void SetActive(EntityTarget target, bool active);

        //! @brief Keep identity and authored values while releasing runtime state and residency, descendants first.
        void Unload(EntityTarget target);

        //! @brief Remove the residency group and cancel its pending creations.
        void UnloadRegistry(EntityRegistry& registry);

        void RemoveComponent(EntityTarget target, Rtti::TypeID type);

        template<class T>
        void RemoveComponent(const EntityTarget target)
        {
            RemoveComponent(target, Rtti::GetTypeID<T>());
        }

        //! @brief Own an authored value until commit; adding an existing type requests an atomic replacement.
        template<class T>
        bool AddComponent(const EntityTarget target, T&& value)
        {
            using Value = std::remove_cvref_t<T>;
            const auto* info = Register<Value>();
            if (!info)
                return false;

            void* storage = AllocatePayload(info->m_type->m_size, info->m_type->m_alignment);
            ::new (storage) Value(std::forward<T>(value));
            RecordComponent(target, *info, storage);
            return true;
        }

        template<class T>
        bool AddComponent(EntityTarget target)
        {
            return AddComponent(target, T{});
        }

        //! @brief Replace an authored value; the previous active value remains until loading succeeds.
        template<class T>
        bool ReplaceComponent(EntityTarget target, T&& value)
        {
            return AddComponent(target, std::forward<T>(value));
        }

    private:
        friend EntityWorld;
        struct Impl;
        Impl* m_impl = nullptr;

        EntityComponentRegistry& Components();

        template<class T>
        const EntityComponentInfo* Register()
        {
            return Components().Register<T>() ? Components().Find(Rtti::GetTypeID<T>()) : nullptr;
        }

        void* AllocatePayload(size_t size, size_t alignment);
        void RecordComponent(EntityTarget target, const EntityComponentInfo& info, void* storage);
    };
} // namespace FE::Framework
