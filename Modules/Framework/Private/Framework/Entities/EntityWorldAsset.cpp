#include <Framework/Entities/EntityWorldAsset.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    namespace
    {
        //! @brief Validate every group and require unique registry keys and entity UUIDs across the whole world.
        template<class Group>
        bool ValidateGroups(const festd::vector<Group>& groups)
        {
            festd::unordered_dense_set<Uuid> keys;
            festd::unordered_dense_set<Uuid> identities;
            for (const auto& group : groups)
            {
                if (!group.m_key.IsValid() || !keys.insert(group.m_key).second || !group.m_entities.Validate())
                    return false;

                for (const auto& entity : group.m_entities.m_entities)
                {
                    if (!identities.insert(entity.m_uuid).second)
                        return false;
                }
            }

            return true;
        }


        bool ValidateBindings(const EntityPlacementSnapshot& placement)
        {
            festd::unordered_dense_set<Uuid> sources;
            festd::unordered_dense_set<Uuid> targets;
            for (const auto& binding : placement.m_bindings)
            {
                if (!binding.m_sourceUuid.IsValid() || !binding.m_entityUuid.IsValid()
                    || binding.m_entityUuid == placement.m_rootUuid)
                {
                    return false;
                }

                if (!sources.insert(binding.m_sourceUuid).second || !targets.insert(binding.m_entityUuid).second)
                    return false;
            }

            return true;
        }
    } // namespace


    bool EntityWorldAsset::Validate() const
    {
        if (!ValidateGroups(m_registries))
            return false;

        festd::unordered_dense_set<IO::AssetID> placements;
        for (const auto& group : m_registries)
        {
            for (const auto& placement : group.m_placements)
            {
                const auto id = placement.GetAssetID();
                if (!id.IsValid() || !placements.insert(id).second)
                    return false;
            }
        }

        return true;
    }


    bool EntityWorldSnapshotAsset::Validate() const
    {
        if (!ValidateGroups(m_registries))
            return false;

        festd::unordered_dense_set<IO::AssetID> assets;
        for (const auto& group : m_registries)
        {
            festd::unordered_dense_set<Uuid> entities;
            for (const auto& entity : group.m_entities.m_entities)
                entities.insert(entity.m_uuid);

            festd::unordered_dense_set<Uuid> owned;
            for (const auto& placement : group.m_placements)
            {
                if (!placement.m_asset.IsValid() || !assets.insert(placement.m_asset).second)
                    return false;

                if (festd::find(placement.m_members, placement.m_rootUuid) == placement.m_members.end())
                    return false;

                // Members must be saved entities of this group and belong to only one placement.
                for (const Uuid member : placement.m_members)
                {
                    if (!entities.contains(member) || !owned.insert(member).second)
                        return false;
                }

                if (!ValidateBindings(placement))
                    return false;
            }
        }

        return true;
    }
} // namespace FE::Framework
