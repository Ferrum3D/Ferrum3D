#pragma once
#include <Framework/Entities/EntityCollection.h>
#include <festd/string.h>

namespace FE::Framework
{
    struct Archetype;


    struct EntityMaterialization
    {
        EntityCollectionInstanceAsset m_definition;
        IO::AssetID m_asset = IO::AssetID::kNull;
        EntityCollection m_collection;
        festd::vector<EntityUuidBinding> m_bindings;
        festd::vector<EntityID> m_entities;
        festd::vector<Archetype*> m_schemas;
        Uuid m_rootUuid = Uuid::kNull;
        uint32_t m_loadCount = 0;
        festd::string m_error;
        IO::AssetRequest m_request;
        EntityRegistry* m_registry = nullptr;
        uint64_t m_eligibleEpoch = 0;
        EntityID m_root;
        uint32_t m_registryId = 0;
        MaterializationState m_state = MaterializationState::kPending;
        bool m_concrete = false;
        bool m_placement = false;
        bool m_hasDefinition = false;
        bool m_validated = false;
        bool m_installed = false;
        bool m_hierarchyInstalled = false;
    };

} // namespace FE::Framework
