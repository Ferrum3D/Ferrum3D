#pragma once
#include <Framework/Entities/Base.h>
#include <Framework/Entities/EntityAssetRecords.h>

namespace FE::Framework
{
    void RegisterEntityAssetStreamers();
    void UnregisterEntityAssetStreamers();


    struct EntityCollection final
    {
        festd::vector<EntityRecord> m_entities;
        festd::vector<uint8_t> m_payload;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000015");
        FE_RTTI_Serialize();
        bool CookComponent(EntityRecord& entity, const Rtti::Type& type, const void* value);
        template<class T>
        bool CookComponent(EntityRecord& entity, const T& value)
        {
            return CookComponent(entity, Rtti::GetType<T>(), &value);
        }
        [[nodiscard]] bool Validate() const;
        [[nodiscard]] bool ValidatePayloads() const;
    };


    struct EntityCollectionInstanceAsset final
    {
        Uuid m_rootUuid = Uuid::kNull;
        IO::Link<EntityCollection> m_collection;
        festd::vector<EntityUuidBinding> m_bindings;
        // Generic authored root components; GameFramework supplies placement transforms here.
        EntityCollection m_root;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000016");
        FE_RTTI_Serialize();
        bool UpdateBindings(const EntityCollection& collection);
        bool MakeIndependentCopy();
        [[nodiscard]] bool Validate() const;
        [[nodiscard]] bool Validate(const EntityCollection& collection) const;
    };


    enum class MaterializationState : uint8_t
    {
        kPending,
        kReady,
        kFailed,
        kCanceled
    };


    struct MaterializationToken final
    {
        uint16_t m_world = 0;
        uint32_t m_index = UINT32_MAX;
        bool operator==(const MaterializationToken&) const = default;
    };


    struct MaterializationStatus final
    {
        MaterializationState m_state = MaterializationState::kFailed;
        EntityID m_root;
        festd::ascii_view m_error;
    };
} // namespace FE::Framework
