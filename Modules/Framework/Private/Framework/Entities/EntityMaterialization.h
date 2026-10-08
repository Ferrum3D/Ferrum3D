#pragma once
#include <Framework/Entities/EntityCollection.h>

namespace FE::Framework
{
    struct EntityMaterialization
    {
        EntityCollectionInstanceAsset m_definition;
        IO::AssetID m_asset = IO::AssetID::kNull;
        EntityCollection m_collection;
        festd::vector<EntityUuidBinding> m_bindings;
        festd::vector<EntityID> m_entities;
        festd::ascii_view m_error;
        IO::AssetRequest m_request;
        IO::AssetRequest m_collectionRequest;
        EntityRegistry* m_registry = nullptr;
        uint64_t m_registryId = 0;
        uint64_t m_eligibleEpoch = 0;
        EntityID m_root;
        MaterializationState m_state = MaterializationState::kPending;
        bool m_placement = false;
        bool m_hasDefinition = false;
    };

} // namespace FE::Framework
