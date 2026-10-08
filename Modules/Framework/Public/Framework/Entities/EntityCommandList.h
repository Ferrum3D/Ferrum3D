#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    //! @brief List-local creation token; it cannot be used by a different recorder.
    struct EntityToken final
    {
        //! @brief Owning recorder identity; tokens cannot cross command lists.
        uint64_t m_list = 0;
        //! @brief World operation or list-local creation index.
        uint32_t m_index = kInvalidIndex;
    };


    //! @brief Command target containing either a runtime ID or a list-local creation token.
    struct EntityTarget final
    {
        //! @brief Stable identity key.
        EntityID m_id;
        //! @brief List-local target identity when no runtime ID is present.
        EntityToken m_token;
        //! @brief Create an empty target; it is used to detach a parent link.
        EntityTarget() = default;

        //! @brief Target an existing generation-checked entity.
        EntityTarget(EntityID id)
            : m_id(id)
        {
        }

        //! @brief Target an entity created earlier by the same command list.
        EntityTarget(EntityToken token)
            : m_token(token)
        {
        }
    };


    // One recorder owns a list. Submit transfers ownership; tokens cannot refer to a different list.
    // Within a list rename, parent and component replacement use the final recorded value.
    // Destroy discards later operations on its target. Independent conflicting lists are rejected.
    //! @brief Single-recorder transactional batch; independent conflicting batches are rejected.
    struct EntityCommandList final
    {
        //! @brief Create a single-recorder batch for one world; Submit consumes it.
        explicit EntityCommandList(EntityWorld& world);
        //! @brief Destroy unsubmitted values and release the batch arena.
        ~EntityCommandList();
        //! @brief Transfer unsubmitted command ownership; the source becomes empty.
        EntityCommandList(EntityCommandList&&) noexcept;
        //! @brief Release this unsubmitted batch and transfer the source batch ownership.
        EntityCommandList& operator=(EntityCommandList&&) noexcept;
        EntityCommandList(const EntityCommandList&) = delete;
        EntityCommandList& operator=(const EntityCommandList&) = delete;
        //! @brief Record an inactive entity creation in a same-world registry; a null UUID generates a random identity.
        EntityToken CreateEntity(EntityRegistry& registry, Env::Name name = {}, Uuid uuid = Uuid::kNull,
                                 ResidencyScope residency = ResidencyScope::kEntity);
        //! @brief Destroy the target and descendants; later operations on the destroyed target are discarded.
        void Destroy(EntityTarget target);
        //! @brief Record the final name for the target in this batch.
        void Rename(EntityTarget target, Env::Name name);
        //! @brief Record final hierarchy and local/world preservation; an empty parent detaches the target.
        void SetParent(EntityTarget target, EntityTarget parent = {}, ReparentMode mode = ReparentMode::kPreserveWorld);
        //! @brief Request activation or deactivation of the target subtree.
        void SetActive(EntityTarget target, bool active);
        //! @brief Keep identity and authored values while releasing runtime state and residency, descendants first.
        void Unload(EntityTarget target);
        //! @brief Remove the residency group and cancel its pending creations.
        void UnloadRegistry(EntityRegistry& registry);
        //! @brief Remove the component type, if present, at commit.
        void RemoveComponent(EntityTarget target, Rtti::TypeID type);
        //! @brief Remove the component type, if present, at commit.
        template<class T>
        void RemoveComponent(EntityTarget target)
        {
            //! @brief Remove the component type, if present, at commit.
            RemoveComponent(target, Rtti::GetTypeID<T>());
        }

        //! @brief Own an authored value until commit; adding an existing type requests an atomic replacement.
        template<class T>
        bool AddComponent(EntityTarget target, T&& value)
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

        //! @brief Own an authored value until commit; adding an existing type requests an atomic replacement.
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
