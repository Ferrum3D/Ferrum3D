#include <FeCore/Console/FeLog.h>
#include <FeCore/ECS/ComponentStorage.h>

namespace FE::ECS
{
    void ComponentStorage::Init(const ComponentStorageDesc& desc)
    {
        FE_ASSERT_MSG(m_Data.Empty(), "Component storage can only be initialized once");
        m_Desc = desc;
        m_Data = desc.Data;
    }

    bool ComponentStorage::AllocateComponentImpl(UInt32& id)
    {
        if (m_Count == m_Data.Length())
        {
            return false;
        }

        id = m_Count++;
        return true;
    }

    void ComponentStorage::UpdateComponentImpl(const void* componentData, UInt32 id)
    {
        memcpy(&m_Data[id * ElementSize()], componentData, m_Desc.Type.DataSize);
    }

    void ComponentStorage::RemoveComponent(UInt32 id)
    {
        if (m_Count == 1)
        {
            m_Count = 0;
        }
        else
        {
            UpdateComponentImpl(&m_Data[m_Data.Length() - ElementSize()], id);
            --m_Count;
        }
    }

    void ComponentStorage::ComponentData(UInt32 id, void** componentData)
    {
        *componentData = &m_Data[id * ElementSize()];
    }
} // namespace FE::ECS
