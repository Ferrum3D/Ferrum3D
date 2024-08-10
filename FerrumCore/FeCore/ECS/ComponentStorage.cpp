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

    bool ComponentStorage::AllocateComponentImpl(uint32_t& id)
    {
        if (m_Count == m_Data.Length())
        {
            return false;
        }

        id = m_Count++;
        return true;
    }

    void ComponentStorage::UpdateComponentImpl(const void* componentData, uint32_t id)
    {
        memcpy(&m_Data[id * ElementSize()], componentData, m_Desc.Type.DataSize);
    }

    int32_t ComponentStorage::RemoveComponent(uint32_t id)
    {
        int32_t result = -1;
        if (id < m_Count - 1)
        {
            result = static_cast<uint16_t>(m_Count - 1);
            UpdateComponentImpl(&m_Data[result * ElementSize()], id);
        }

        --m_Count;

        return result;
    }

    void ComponentStorage::ComponentData(uint32_t id, void** componentData)
    {
        *componentData = &m_Data[id * ElementSize()];
    }
} // namespace FE::ECS
