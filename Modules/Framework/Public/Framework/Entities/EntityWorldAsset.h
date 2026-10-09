#pragma once
#include <Framework/Entities/EntityCollection.h>

namespace FE::Framework
{
    //! @brief One ownership group in an authored world; its key survives save/restore and streaming.
    struct EntityWorldRegistryDefinition final
    {
        //! @brief Persistent ownership identity; independent runtime worlds may reuse this key.
        Uuid m_key = Uuid::kNull;
        //! @brief Concrete authored or saved values, with UUIDs preserved during materialization.
        EntityCollection m_entities;
        //! @brief Soft placement definitions loaded explicitly by world materialization or streaming.
        festd::vector<IO::Link<EntityCollectionInstanceAsset, IO::DependencyKind::kSoft>> m_placements;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae0901");
        FE_RTTI_Serialize();
    };


    //! @brief Reusable world definition; systems and services are default-constructed through registered reflection.
    struct EntityWorldAsset final
    {
        //! @brief Reflected, default-constructible WorldSystem types in initialization order.
        festd::vector<Rtti::TypeID> m_systems;
        //! @brief Reflected, default-constructible WorldService types initialized before systems.
        festd::vector<Rtti::TypeID> m_services;
        //! @brief Ownership groups; empty groups are preserved.
        festd::vector<EntityWorldRegistryDefinition> m_registries;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae0902");
        FE_RTTI_Serialize();
        //! @brief Validate keys, concrete identities, hierarchy and cooked component envelopes.
        [[nodiscard]] bool Validate() const;
    };


    //! @brief Persistent placement ownership without a source collection expansion on restore.
    struct EntityPlacementSnapshot final
    {
        //! @brief Placement identity used for deduplication, without retaining its source asset.
        IO::AssetID m_asset = IO::AssetID::kNull;
        //! @brief Concrete placement root included in the surviving membership.
        Uuid m_rootUuid = Uuid::kNull;
        //! @brief Original source bindings, including source rows deleted at runtime.
        festd::vector<EntityUuidBinding> m_bindings;
        //! @brief Surviving concrete membership; restored without expanding source collections.
        festd::vector<Uuid> m_members;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae0903");
        FE_RTTI_Serialize();
    };


    //! @brief Concrete saved ownership group, including empty groups and surviving placement membership.
    struct EntityWorldRegistrySnapshot final
    {
        //! @brief Persistent ownership identity; independent runtime worlds may reuse this key.
        Uuid m_key = Uuid::kNull;
        //! @brief Concrete authored or saved values, with UUIDs preserved during materialization.
        EntityCollection m_entities;
        //! @brief Original placement identities/bindings and their surviving concrete membership.
        festd::vector<EntityPlacementSnapshot> m_placements;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae0904");
        FE_RTTI_Serialize();
    };


    //! @brief Complete concrete world state; runtime handles and transient component columns are excluded.
    struct EntityWorldSnapshotAsset final
    {
        //! @brief Reflected, default-constructible WorldSystem types in initialization order.
        festd::vector<Rtti::TypeID> m_systems;
        //! @brief Reflected, default-constructible WorldService types initialized before systems.
        festd::vector<Rtti::TypeID> m_services;
        //! @brief Ownership groups; empty groups are preserved.
        festd::vector<EntityWorldRegistrySnapshot> m_registries;
        FE_RTTI_Reflect("aaa69126-4427-4055-b1c4-04013aae0905");
        FE_RTTI_Serialize();
        //! @brief Validate concrete records and saved membership before restoration changes a world.
        [[nodiscard]] bool Validate() const;
    };
} // namespace FE::Framework
