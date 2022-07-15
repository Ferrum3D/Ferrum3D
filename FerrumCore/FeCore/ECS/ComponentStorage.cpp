#include <FeCore/Console/FeLog.h>
#include <FeCore/ECS/ComponentStorage.h>

namespace FE::ECS
{
    void ComponentStorage::Init(const ComponentStorageDesc& desc)
    {
        FE_ASSERT_MSG(m_Pages.Empty(), "Component storage can only be initialized once");
        m_Desc = desc;

        m_Pages.Emplace().Init(SparseStorageDesc(
            m_Desc.Type.Alignment, m_Desc.Type.DataSize, SparseStorageAllocationPolicy::PreAllocate, ComponentCountPerPage));
    }

    ECSResult ComponentStorage::AllocateComponent(void* componentData, UInt32& id)
    {
        for (USize i = 0; i < m_Pages.Size(); ++i)
        {
            for (USize j = 0; j < ComponentCountPerPage; ++j)
            {
                if (m_Pages[i].Insert(static_cast<UInt32>(j), componentData))
                {
                    id = static_cast<UInt32>((i << ComponentCountPerPageLog2) | j);
                    return ECSResult::Success;
                }
            }
        }

        // All pages are out of memory, so we need a new one
        id = static_cast<UInt32>(m_Pages.Size() << ComponentCountPerPageLog2);
        m_Pages.Emplace().Init(SparseStorageDesc(
            m_Desc.Type.Alignment, m_Desc.Type.DataSize, SparseStorageAllocationPolicy::PreAllocate, ComponentCountPerPage));

        return m_Pages.Back().Insert(0, componentData) ? ECSResult::Success : ECSResult::AllocationError;
    }

    ECSResult ComponentStorage::UpdateComponent(void* componentData, UInt32 id)
    {
        auto pageIndex = id >> ComponentCountPerPageLog2;
        auto compIndex = id & ComponentCountPerPageMask;

        if (void* data = m_Pages[pageIndex].TryGetAt(compIndex))
        {
            memcpy(data, componentData, m_Desc.Type.DataSize);
            return ECSResult::Success;
        }

        return ECSResult::EntityNotFound;
    }

    ECSResult ComponentStorage::RemoveComponent(UInt32 id)
    {
        auto pageIndex = id >> ComponentCountPerPageLog2;
        auto compIndex = id & ComponentCountPerPageMask;

        if (m_Pages[pageIndex].Remove(compIndex))
        {
            if (m_FreeListSize < FreeListCapacity)
            {
                m_FreeList[m_FreeListSize++] = id;
            }

            return ECSResult::Success;
        }

        return ECSResult::EntityNotFound;
    }
} // namespace FE::ECS
