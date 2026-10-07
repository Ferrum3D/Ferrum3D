#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    struct EntityToken final
    {
        uint64_t m_list = 0;
        uint32_t m_index = kInvalidIndex;
    };


    struct EntityTarget final
    {
        EntityID m_id;
        EntityToken m_token;
        EntityTarget() = default;
        EntityTarget(EntityID id)
            : m_id(id)
        {
        }


        EntityTarget(EntityToken token)
            : m_token(token)
        {
        }
    };


    // One recorder owns a list. Submit transfers ownership; tokens cannot refer to a different list.
    // Within a list rename, parent and component replacement use the final recorded value.
    // Destroy discards later operations on its target. Independent conflicting lists are rejected.
    struct EntityCommandList final
    {
        explicit EntityCommandList(EntityWorld& world);
        ~EntityCommandList();
        EntityCommandList(EntityCommandList&&) noexcept;
        EntityCommandList& operator=(EntityCommandList&&) noexcept;
        EntityCommandList(const EntityCommandList&) = delete;
        EntityCommandList& operator=(const EntityCommandList&) = delete;
        EntityToken CreateEntity(EntityRegistry& registry, Env::Name name = {}, Uuid uuid = Uuid::kNull,
                                 ResidencyScope residency = ResidencyScope::kEntity);
        void Destroy(EntityTarget target);
        void Rename(EntityTarget target, Env::Name name);
        void SetParent(EntityTarget target, EntityTarget parent = {});
        void SetActive(EntityTarget target, bool active);
        void Unload(EntityTarget target);
        void UnloadRegistry(EntityRegistry& registry);
        void RemoveComponent(EntityTarget target, Rtti::TypeID type);
        template<class T>
        void RemoveComponent(EntityTarget target)
        {
            RemoveComponent(target, Rtti::GetTypeID<T>());
        }


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


        template<class T>
        bool AddComponent(EntityTarget target)
        {
            return AddComponent(target, T{});
        }


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
