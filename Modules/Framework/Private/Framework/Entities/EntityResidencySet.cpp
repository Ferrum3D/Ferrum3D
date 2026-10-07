#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    EntityAssetRequest EntityAssetServices::Acquire(const IO::AssetID id, const Rtti::TypeID)
    {
        return { IO::AssetManager::LoadAsset(id), 0 };
    }


    LifecycleResult EntityAssetServices::Poll(const EntityAssetRequest& request, const Rtti::TypeID expectedType)
    {
        const auto result = request.m_request.GetResult();
        if (result == IO::AssetLoadResult::kPending)
            return LifecycleResult::kPending;

        if (result != IO::AssetLoadResult::kSucceeded)
            return LifecycleResult::kFailed;

        const auto* slot = request.m_request.GetAssetSlot();
        if (expectedType.IsValid() && (!slot || slot->m_typeId != expectedType))
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


    bool EntityResidencySet::Add(const IO::AssetID id, const Rtti::TypeID expectedType)
    {
        if (!id.IsValid())
            return true;

        for (auto& entry : m_entries)
        {
            if (entry.m_id == id)
            {
                ++entry.m_contributors;
                return true;
            }
        }

        m_entries.push_back({ id, expectedType, m_services->Acquire(id, expectedType), 1 });
        return true;
    }


    void EntityResidencySet::Remove(const IO::AssetID id, const Rtti::TypeID)
    {
        for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            if (it->m_id != id)
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
            if (entry.m_id == id)
                return m_services->Poll(entry.m_request, expectedType);
        }

        return LifecycleResult::kFailed;
    }


    bool ComponentLoadingContext::Require(const IO::AssetID id, const Rtti::TypeID expectedType) const
    {
        return m_world.RequireAsset(m_entity, id, expectedType);
    }
} // namespace FE::Framework
