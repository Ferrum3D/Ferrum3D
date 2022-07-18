#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    EntityArchetype::EntityArchetype(const ArraySlice<ComponentType>& layout)
    {
        m_Layout.Assign(layout.Data(), layout.Data() + layout.Length());
        InitInternal();
    }

    void EntityArchetype::InitInternal()
    {
        m_Layout.SortByMember(&ComponentType::Alignment, true);

        for (USize i = 0; i < m_Layout.Size(); ++i)
        {
            FE_ASSERT_MSG(i == 0 || m_Layout[i].Type != m_Layout[i - 1].Type,
                          "Same component type added twice to the same archetype");
            m_EntitySize += m_Layout[i].AlignedSize();
            HashCombine(m_HashCode, m_Layout[i]);
        }
    }

    EntityArchetype::~EntityArchetype()
    {
        for (auto* chunk : m_Chunks)
        {
            chunk->Deallocate();
        }
    }

    ArchetypeChunk* EntityArchetype::AllocateChunk()
    {
        ArchetypeChunkDesc desc{};
        desc.Archetype = this;
        m_Chunks.Emplace(ArchetypeChunk::Allocate())->Init(desc);
        return m_Chunks.Back();
    }

    void EntityArchetype::DeallocateChunk(ArchetypeChunk* chunk)
    {
        auto idx = m_Chunks.IndexOf(chunk);
        FE_ASSERT_MSG(idx != 1, "Couldn't find ArchetypeChunk: {}", chunk);
        m_Chunks[idx]->Deallocate();
        m_Chunks.SwapRemoveAt(idx);
    }

    EntityArchetypeMatch EntityArchetype::Match(const EntityArchetype& other)
    {
        USize matchCount = 0;
        for (USize i = 0; i < ComponentTypeCount(); ++i)
        {
            for (USize j = 0; j < other.ComponentTypeCount(); ++j)
            {
                if (m_Layout[i].Type == other.m_Layout[i].Type)
                {
                    ++matchCount;
                }
            }
        }

        if (matchCount == 0)
        {
            return EntityArchetypeMatch::None;
        }

        return matchCount == other.ComponentTypeCount() ? EntityArchetypeMatch::All : EntityArchetypeMatch::Some;
    }

    bool operator==(const EntityArchetype& lhs, const EntityArchetype& rhs)
    {
        if (lhs.m_HashCode != rhs.m_HashCode || lhs.ComponentTypeCount() != rhs.ComponentTypeCount())
        {
            return false;
        }

        for (USize i = 0; i < lhs.ComponentTypeCount(); ++i)
        {
            // This is valid since all component types are sorted
            if (lhs.m_Layout[i].Type != rhs.m_Layout[i].Type)
            {
                return false;
            }
        }

        return true;
    }
} // namespace FE::ECS
