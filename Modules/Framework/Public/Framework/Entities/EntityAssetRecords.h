#pragma once
#include <Core/IO/Assets.h>
#include <festd/string.h>
#include <festd/vector.h>

namespace FE::Framework
{
    // Authored data contracts for the asset-materialization milestone. Runtime indices/handles are never stored here.
    struct EntityDependencyRecord final
    {
        IO::AssetID m_asset = IO::AssetID::kNull;
        Rtti::TypeID m_expectedType = Rtti::TypeID::kNull;
        IO::DependencyKind m_kind = IO::DependencyKind::kHard;
    };


    struct EntityComponentRecord final
    {
        Rtti::TypeID m_type = Rtti::TypeID::kNull;
        uint32_t m_version = 0;
        uint64_t m_schemaHash = 0;
        uint32_t m_payloadOffset = 0;
        uint32_t m_payloadSize = 0;
        festd::inline_vector<EntityDependencyRecord> m_dependencies;
    };


    struct EntityRecord final
    {
        Uuid m_uuid = Uuid::kNull;
        Uuid m_parentUuid = Uuid::kNull;
        festd::string m_name;
        festd::inline_vector<EntityComponentRecord> m_components;
    };


    struct EntityUuidBinding final
    {
        Uuid m_sourceUuid = Uuid::kNull;
        Uuid m_entityUuid = Uuid::kNull;
    };


    struct EntityCollection;
    struct EntityCollectionInstanceAsset;
    struct EntityWorldAsset;
    struct EntityWorldSnapshotAsset;
} // namespace FE::Framework
