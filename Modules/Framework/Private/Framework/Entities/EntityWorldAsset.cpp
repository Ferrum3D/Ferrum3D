#include <Framework/Entities/EntityWorldAsset.h>

namespace FE::Framework
{
    namespace
    {
        template<class Group>
        bool ValidateGroups(const festd::vector<Group>& groups)
        {
            festd::vector<Uuid> identities;
            for (uint32_t i = 0; i < groups.size(); ++i)
            {
                const auto& group = groups[i];
                if (!group.m_key.IsValid() || !group.m_entities.ValidatePayloads())
                    return false;

                for (uint32_t j = 0; j < i; ++j)
                {
                    if (groups[j].m_key == group.m_key)
                        return false;
                }

                for (const auto& entity : group.m_entities.m_entities)
                {
                    if (festd::find(identities, entity.m_uuid) != identities.end())
                        return false;

                    identities.push_back(entity.m_uuid);
                }
            }

            return true;
        }
    } // namespace


    bool EntityWorldAsset::Validate() const
    {
        if (!ValidateGroups(m_registries))
            return false;

        festd::vector<IO::AssetID> placements;
        for (const auto& group : m_registries)
        {
            for (const auto& placement : group.m_placements)
            {
                const auto id = placement.GetAssetID();
                if (!id.IsValid() || festd::find(placements, id) != placements.end())
                    return false;

                placements.push_back(id);
            }
        }

        return true;
    }


    bool EntityWorldSnapshotAsset::Validate() const
    {
        if (!ValidateGroups(m_registries))
            return false;

        festd::vector<IO::AssetID> assets;
        for (const auto& group : m_registries)
        {
            festd::vector<Uuid> owned;
            for (const auto& placement : group.m_placements)
            {
                if (!placement.m_asset.IsValid() || festd::find(assets, placement.m_asset) != assets.end())
                    return false;

                assets.push_back(placement.m_asset);
                if (festd::find(placement.m_members, placement.m_rootUuid) == placement.m_members.end())
                    return false;

                for (const Uuid member : placement.m_members)
                {
                    const auto found = festd::find_if(group.m_entities.m_entities.begin(),
                                                      group.m_entities.m_entities.end(),
                                                      [member](const EntityRecord& entity) {
                                                          return entity.m_uuid == member;
                                                      });
                    if (found == group.m_entities.m_entities.end() || festd::find(owned, member) != owned.end())
                        return false;

                    owned.push_back(member);
                }

                for (uint32_t i = 0; i < placement.m_bindings.size(); ++i)
                {
                    const auto& binding = placement.m_bindings[i];
                    if (!binding.m_sourceUuid.IsValid() || !binding.m_entityUuid.IsValid()
                        || binding.m_entityUuid == placement.m_rootUuid)
                    {
                        return false;
                    }

                    for (uint32_t j = 0; j < i; ++j)
                    {
                        if (placement.m_bindings[j].m_sourceUuid == binding.m_sourceUuid
                            || placement.m_bindings[j].m_entityUuid == binding.m_entityUuid)
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }
} // namespace FE::Framework
