#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    void EntityComponentRegistry::RegisterEntry(const EntityComponentInfo* entry)
    {
        std::lock_guard lock{ m_lock };
        m_entries[entry->m_typeID] = entry;
    }


    const EntityComponentInfo* EntityComponentRegistry::GetComponentInfo(const ComponentTypeID typeID) const
    {
        std::lock_guard lock{ m_lock };
        const auto it = m_entries.find(typeID);
        FE_Assert(it != m_entries.end());
        return it->second;
    }


    EntityComponentRegistry& EntityComponentRegistry::Get()
    {
        static EntityComponentRegistry registry;
        return registry;
    }
} // namespace FE::Framework
