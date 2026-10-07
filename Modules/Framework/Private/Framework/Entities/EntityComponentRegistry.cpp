#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    EntityComponentRegistry::~EntityComponentRegistry()
    {
        for (auto* entry : m_entries)
            Memory::DefaultDelete(entry);
    }


    const EntityComponentInfo* EntityComponentRegistry::Find(const Rtti::TypeID id) const
    {
        std::lock_guard lock{ m_lock };
        return FindUnlocked(id);
    }


    const EntityComponentInfo* EntityComponentRegistry::FindUnlocked(const Rtti::TypeID id) const
    {
        for (const auto* entry : m_entries)
        {
            if (entry->m_type->m_id == id)
                return entry;
        }

        return nullptr;
    }
} // namespace FE::Framework
