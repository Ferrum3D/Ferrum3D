#pragma once
#include <Framework/Entities/Base.h>
#include <Framework/Entities/EntityAssetRecords.h>

namespace FE::Framework
{
    //! @brief Engine-independent entity hierarchy with reflected component values.
    struct EntityCollection final
    {
        //! @brief Authored entity identities, hierarchy, and reflected component values.
        festd::vector<EntityRecord> m_entities;

        FE_RTTI_Reflect("8837564E-1522-403E-9A73-E5B8C4BE1EF6");
        FE_RTTI_Serialize();

        //! @brief Check identities, hierarchy, and component types without asserting on invalid content.
        [[nodiscard]] bool Validate() const;
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
        //! @brief Authored placement root components.
        EntityCollection m_root;

        FE_RTTI_Reflect("DEE5027D-8FFF-433B-B9AF-E220068F94D5");
        FE_RTTI_Serialize();

        //! @brief Preserve existing source bindings and generate identities for newly added sources; reject invalid content.
        bool UpdateBindings(const EntityCollection& collection);
        //! @brief Generate fresh root and bound identities and remap internal root references; commit only on success.
        bool MakeIndependentCopy();

        //! @brief Check root identity, root components and binding uniqueness without asserting on content.
        [[nodiscard]] bool Validate() const;
        //! @brief Check this placement, the collection, and that bindings map every collection source exactly once.
        [[nodiscard]] bool Validate(const EntityCollection& collection) const;
        //! @brief Check only that bindings map every source of an already validated collection exactly once.
        [[nodiscard]] bool Matches(const EntityCollection& collection) const;
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
        uint16_t m_world = 0;
        uint32_t m_index = UINT32_MAX;

        bool operator==(const MaterializationToken&) const = default;
    };


    //! @brief Current publication result; pending operations may have an allocated but inactive root.
    struct MaterializationStatus final
    {
        MaterializationState m_state = MaterializationState::kFailed;
        EntityID m_root;
        //! @brief Borrowed recoverable diagnostic text.
        festd::string_view m_error;
    };
} // namespace FE::Framework
