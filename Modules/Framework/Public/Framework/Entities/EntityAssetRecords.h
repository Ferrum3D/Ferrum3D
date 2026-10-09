#pragma once
#include <Core/IO/Assets.h>
#include <Framework/Entities/Base.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Framework
{
    // Authored identities and cooked envelopes never contain runtime handles.
    //! @brief Cooked dependency declaration replayed by asset build and runtime residency.
    struct EntityDependencyRecord final
    {
        //! @brief Logical asset identity; null represents no dependency.
        IO::AssetID m_asset = IO::AssetID::kNull;
        //! @brief Expected reflected asset type; null accepts the discovered type.
        Rtti::TypeID m_expectedType = Rtti::TypeID::kNull;
        //! @brief Build/runtime dependency strength.
        IO::DependencyKind m_kind = IO::DependencyKind::kHard;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000010");
        FE_RTTI_Serialize();
        //! @brief Report the dependency to import/build discovery using its declared strength and expected type.
        void BeforeSerialize(Serialization::SerializationContext& context) const;
    };


    //! @brief Schema-checked component envelope referencing a byte range in its collection payload.
    struct EntityComponentRecord final
    {
        //! @brief Reflected component identity or stable RTTI metadata.
        Rtti::TypeID m_type = Rtti::TypeID::kNull;
        //! @brief Cooked asset dependency envelope for this component.
        festd::vector<EntityDependencyRecord> m_dependencies;
        //! @brief Generated serialization schema hash of the cooked value.
        uint64_t m_schemaHash = 0;
        //! @brief Cooked serialization version or last consumed change version.
        uint32_t m_version = 0;
        //! @brief Byte offset into the collection payload.
        uint32_t m_payloadOffset = 0;
        //! @brief Serialized byte count at the payload offset.
        uint32_t m_payloadSize = 0;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000013");
        FE_RTTI_Serialize();
    };


    //! @brief Authored source identity, parent, name, and component envelopes; contains no runtime handles.
    struct EntityRecord final
    {
        //! @brief Authored or referenced UUID; never a runtime entity handle.
        Uuid m_uuid = Uuid::kNull;
        //! @brief Source UUID of the authored parent; null identifies a collection root.
        Uuid m_parentUuid = Uuid::kNull;
        //! @brief Display name; not an identity key.
        festd::string m_name;
        //! @brief Component envelopes in authored order.
        festd::inline_vector<EntityComponentRecord> m_components;
        //! @brief Saved activation intent; inactive records remain allocated and excluded from queries.
        bool m_active = true;
        //! @brief Saved dependency deduplication scope.
        ResidencyScope m_residency = ResidencyScope::kEntity;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000011");
        FE_RTTI_Serialize();
    };


    //! @brief Map one collection source UUID to one concrete placement/runtime UUID.
    struct EntityUuidBinding final
    {
        //! @brief UUID used by the source collection.
        Uuid m_sourceUuid = Uuid::kNull;
        //! @brief Concrete UUID allocated for this placement or spawn.
        Uuid m_entityUuid = Uuid::kNull;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000012");
        FE_RTTI_Serialize();
    };


    //! @brief Cooked engine-independent entity hierarchy with shared serialized component bytes.
    struct EntityCollection;
    //! @brief Authored placement root and concrete identity bindings for a referenced collection.
    struct EntityCollectionInstanceAsset;
    struct EntityWorldAsset;
    struct EntityWorldSnapshotAsset;
} // namespace FE::Framework
