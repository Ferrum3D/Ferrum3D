#include <Framework/Entities/EntityRegistryInternal.h>

namespace FE::Framework::Internal
{
    RegistryIDPool::~RegistryIDPool()
    {
        for (const uint32_t references : m_references)
            FE_Assert(references == 0, "Command lists must not outlive their world");
    }


    uint32_t RegistryIDPool::Allocate()
    {
        std::lock_guard lock{ m_lock };
        uint32_t id = m_available.find_first();
        if (id == kInvalidIndex)
        {
            const uint32_t oldSize = m_available.size();
            FE_Assert(oldSize <= UINT32_MAX - 64, "Registry ID space exhausted");
            m_available.resize(oldSize + 64, true);
            m_references.resize(m_available.size(), 0);
            if (oldSize == 0)
                m_available.reset(0);

            id = m_available.find_first();
        }

        m_available.reset(id);
        m_references[id] = 1;
        return id;
    }


    void RegistryIDPool::Retain(const uint32_t id)
    {
        std::lock_guard lock{ m_lock };
        FE_AssertDebug(id != 0 && id < m_references.size() && m_references[id] != 0);
        FE_Assert(m_references[id] != UINT32_MAX);
        ++m_references[id];
    }


    void RegistryIDPool::Release(const uint32_t id)
    {
        std::lock_guard lock{ m_lock };
        FE_AssertDebug(id != 0 && id < m_references.size() && m_references[id] != 0);
        if (--m_references[id] == 0)
            m_available.set(id);
    }
} // namespace FE::Framework::Internal
