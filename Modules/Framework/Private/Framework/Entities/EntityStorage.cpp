#include <Framework/Entities/EntityStorage.h>

namespace FE::Framework
{
    ArchetypeChunk& EntityStorage::GetAvailableChunk(Archetype& archetype, EntityRegistry& registry)
    {
        auto& available = m_availableChunks[{ &archetype, &registry }];
        if (available.empty())
        {
            auto* chunk = Memory::DefaultNew<ArchetypeChunk>(archetype, registry);
            m_chunks.push_back(chunk);
            available.push_back(chunk);
        }

        return *available.back();
    }


    void EntityStorage::RowAllocated(ArchetypeChunk& chunk)
    {
        if (chunk.m_count < chunk.m_archetype.m_capacity)
            return;

        const auto found = m_availableChunks.find({ &chunk.m_archetype, &chunk.m_registry });
        FE_AssertDebug(found != m_availableChunks.end() && found->second.back() == &chunk);
        found->second.pop_back();
        if (found->second.empty())
            m_availableChunks.erase(found);
    }


    void EntityStorage::FreeRow(ArchetypeChunk& chunk, const uint32_t row)
    {
        const bool wasFull = chunk.m_count == chunk.m_archetype.m_capacity;
        chunk.Free(row);
        const ChunkKey key{ &chunk.m_archetype, &chunk.m_registry };
        if (chunk.m_count != 0)
        {
            if (wasFull)
                m_availableChunks[key].push_back(&chunk);

            return;
        }

        if (!wasFull)
        {
            const auto found = m_availableChunks.find(key);
            FE_AssertDebug(found != m_availableChunks.end());
            auto& available = found->second;
            available.erase(festd::find(available, &chunk));
            if (available.empty())
                m_availableChunks.erase(found);
        }

        m_chunks.erase(festd::find(m_chunks, &chunk));
        Memory::DefaultDelete(&chunk);
    }
} // namespace FE::Framework
