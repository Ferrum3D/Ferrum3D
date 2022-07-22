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

    Int32 ComponentStorage::RemoveComponent(UInt32 id)
    {
        Int32 result = -1;
        if (id < m_Count - 1)
        {
            result = static_cast<UInt16>(m_Count - 1);
            UpdateComponentImpl(&m_Data[result * ElementSize()], id);
        }

        --m_Count;

        return result;
    }

    void ComponentStorage::ComponentData(UInt32 id, void** componentData)
    {
        *componentData = &m_Data[id * ElementSize()];
    }
} // namespace FE::ECS
