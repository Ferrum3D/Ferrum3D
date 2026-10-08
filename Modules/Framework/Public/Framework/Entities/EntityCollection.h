#pragma once
#include <Framework/Entities/Base.h>
#include <Framework/Entities/EntityAssetRecords.h>

namespace FE::Framework
{
    //! @brief Cooked engine-independent entity hierarchy with shared serialized component bytes.
    struct EntityCollection final
    {
        //! @brief Authored entity records or occupied runtime row owners.
        festd::vector<EntityRecord> m_entities;
        //! @brief Shared cooked component byte storage.
        festd::vector<uint8_t> m_payload;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000015");
        FE_RTTI_Serialize();
        //! @brief Serialize one authored value into the collection payload and record its asset references; failure leaves the entity unchanged.
        bool CookComponent(EntityRecord& entity, const Rtti::Type& type, const void* value);
        //! @brief Serialize one authored value into the collection payload and record its asset references; failure leaves the entity unchanged.
        template<class T>
        bool CookComponent(EntityRecord& entity, const T& value)
        {
            return CookComponent(entity, Rtti::GetType<T>(), &value);
        }
        //! @brief Check authored IDs, hierarchy, envelopes, and placement bindings without asserting on content.
        [[nodiscard]] bool Validate() const;
        //! @brief Deserialize cooked payloads and verify their dependency envelopes; intended for import/build validation.
        [[nodiscard]] bool ValidatePayloads() const;
    };


    //! @brief Authored placement root and concrete identity bindings for a referenced collection.
    struct EntityCollectionInstanceAsset final
    {
        //! @brief Concrete identity of the placement membership root.
        Uuid m_rootUuid = Uuid::kNull;
        //! @brief Non-owning link to the referenced collection definition.
        IO::Link<EntityCollection> m_collection;
        //! @brief Concrete UUID mapping for each collection source.
        festd::vector<EntityUuidBinding> m_bindings;
        // Generic authored root components; GameFramework supplies placement transforms here.
        //! @brief Authored placement root components or runtime membership root ID.
        EntityCollection m_root;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000016");
        FE_RTTI_Serialize();
        //! @brief Preserve existing source bindings and generate identities for newly added sources; reject invalid content.
        bool UpdateBindings(const EntityCollection& collection);
        //! @brief Generate fresh root and bound identities and remap internal root references; commit only on success.
        bool MakeIndependentCopy();
        //! @brief Check authored IDs, hierarchy, envelopes, and placement bindings without asserting on content.
        [[nodiscard]] bool Validate() const;
        //! @brief Check authored IDs, hierarchy, envelopes, and placement bindings without asserting on content.
        [[nodiscard]] bool Validate(const EntityCollection& collection) const;
    };


    //! @brief Asynchronous definition-to-runtime publication state.
    enum class MaterializationState : uint8_t
    {
        //! @brief The operation is not ready; poll it at a later safe point.
        kPending,
        //! @brief The complete membership is prepared and published.
        kReady,
        //! @brief The operation failed and runtime state must be unwound.
        kFailed,
        //! @brief The request was canceled and generated membership released.
        kCanceled
    };


    //! @brief World-local operation identity retained until world destruction.
    struct MaterializationToken final
    {
        //! @brief Owning world or its incarnation; borrowed where represented as a reference.
        uint16_t m_world = 0;
        //! @brief World operation or list-local creation index.
        uint32_t m_index = UINT32_MAX;
        //! @brief Compare operation identity, including its owning world incarnation.
        bool operator==(const MaterializationToken&) const = default;
    };


    //! @brief Current publication result; pending operations may have an allocated but inactive root.
    struct MaterializationStatus final
    {
        //! @brief Current publication result.
        MaterializationState m_state = MaterializationState::kFailed;
        //! @brief Authored placement root components or runtime membership root ID.
        EntityID m_root;
        //! @brief Borrowed recoverable diagnostic text.
        festd::ascii_view m_error;
    };
} // namespace FE::Framework
