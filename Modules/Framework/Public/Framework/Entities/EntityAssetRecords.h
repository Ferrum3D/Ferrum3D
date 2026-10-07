#pragma once
#include <Core/IO/Assets.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Framework
{
    // Authored identities and cooked envelopes never contain runtime handles.
    struct EntityDependencyRecord final
    {
        FE_SERIALIZE_NAME("asset") IO::AssetID m_asset = IO::AssetID::kNull;
        FE_SERIALIZE_NAME("expectedType") Rtti::TypeID m_expectedType = Rtti::TypeID::kNull;
        FE_SERIALIZE_NAME("kind") IO::DependencyKind m_kind = IO::DependencyKind::kHard;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000010");
        FE_RTTI_Serialize();
        void BeforeSerialize(Serialization::SerializationContext& context) const;
    };


    struct EntityComponentRecord final
    {
        Rtti::TypeID m_type = Rtti::TypeID::kNull;
        uint32_t m_version = 0;
        uint64_t m_schemaHash = 0;
        uint32_t m_payloadOffset = 0;
        uint32_t m_payloadSize = 0;
        festd::inline_vector<EntityDependencyRecord> m_dependencies;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000013");
        FE_RTTI_Serialize();
    };


    struct EntityRecord final
    {
        Uuid m_uuid = Uuid::kNull;
        Uuid m_parentUuid = Uuid::kNull;
        festd::string m_name;
        festd::inline_vector<EntityComponentRecord> m_components;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000011");
        FE_RTTI_Serialize();
    };


    struct EntityUuidBinding final
    {
        Uuid m_sourceUuid = Uuid::kNull;
        Uuid m_entityUuid = Uuid::kNull;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000012");
        FE_RTTI_Serialize();
    };


    struct EntityCollection;
    struct EntityCollectionInstanceAsset;
    struct EntityWorldAsset;
    struct EntityWorldSnapshotAsset;
} // namespace FE::Framework
