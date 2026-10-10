#include <Framework/Entities/EntityResidencySet.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    EntityAssetRequest EntityAssetServices::Acquire(const IO::AssetID id, const Rtti::TypeID expectedType)
    {
        return { IO::AssetManager::LoadAsset(id, expectedType), 0 };
    }


    LifecycleResult EntityAssetServices::Poll(const EntityAssetRequest& request, const Rtti::TypeID)
    {
        const auto result = request.m_request.GetResult();
        if (result == IO::AssetLoadResult::kPending)
            return LifecycleResult::kPending;

        if (result != IO::AssetLoadResult::kSucceeded)
            return LifecycleResult::kFailed;

        return LifecycleResult::kSucceeded;
    }


    void EntityAssetServices::Release(EntityAssetRequest& request)
    {
        request.m_request = {};
    }


    EntityResidencySet::~EntityResidencySet()
    {
        for (auto& entry : m_entries)
            m_services->Release(entry.m_request);
    }


    void EntityResidencySet::Add(const IO::AssetID id, const Rtti::TypeID expectedType)
    {
        if (!id.IsValid())
            return;

        for (auto& entry : m_entries)
        {
            if (entry.m_id == id && entry.m_expectedType == expectedType)
            {
                ++entry.m_contributors;
                return;
            }
        }

        m_entries.push_back({ id, expectedType, m_services->Acquire(id, expectedType), 1 });
    }


    void EntityResidencySet::Remove(const IO::AssetID id, const Rtti::TypeID expectedType)
    {
        for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            if (it->m_id != id || it->m_expectedType != expectedType)
                continue;

            if (--it->m_contributors == 0)
            {
                m_services->Release(it->m_request);
                m_entries.erase(it);
            }

            return;
        }
    }


    LifecycleResult EntityResidencySet::Poll(const IO::AssetID id, const Rtti::TypeID expectedType) const
    {
        for (const auto& entry : m_entries)
        {
            if (entry.m_id == id && entry.m_expectedType == expectedType)
                return m_services->Poll(entry.m_request, expectedType);
        }

        return LifecycleResult::kFailed;
    }


    void ComponentLoadingContext::Require(const IO::AssetID id, const Rtti::TypeID expectedType) const
    {
        m_world.RequireAsset(m_entity, id, expectedType);
    }
} // namespace FE::Framework
