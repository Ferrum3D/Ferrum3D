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

    void EntityArchetype::CollectGarbage()
    {
        List<ArchetypeChunk*> newChunks;
        for (ArchetypeChunk*& chunk : m_Chunks)
        {
            if (chunk->Empty())
            {
                chunk->Deallocate();
            }
            else
            {
                newChunks.Push(chunk);
            }
        }

        m_Chunks.Swap(newChunks);
    }

    ECSResult EntityArchetype::CreateEntity(UInt32& id, ArchetypeChunk** chunk)
    {
        ++m_Version;
        for (auto* c : m_Chunks)
        {
            auto result = c->AllocateEntity(id);

            switch (result)
            {
            case ECSResult::Success:
                *chunk = c;
                return ECSResult::Success;
            case ECSResult::OutOfMemoryError:
                continue;
            default:
                FE_UNREACHABLE("Unknown error: {}", static_cast<UInt32>(result));
                continue;
            }
        }

        *chunk = AllocateChunk();
        return (*chunk)->AllocateEntity(id);
    }

    ECSResult EntityArchetype::DestroyEntity(UInt32 id, ArchetypeChunk* chunk)
    {
        if (ValidationEnabled())
        {
            FE_ASSERT_MSG(m_Chunks.IndexOf(chunk) != -1, "The chunk doesn't belong to this archetype.");
        }

        auto result = chunk->DeallocateEntity(id);
        if (result == ECSResult::Success)
        {
            ++m_Version;
        }

        return result;
    }

    ECSResult EntityArchetype::UpdateComponent(UInt32 entityID, ArchetypeChunk* chunk, const TypeID& typeID, const void* source)
    {
        auto result = chunk->UpdateComponent(entityID, typeID, source);
        if (result == ECSResult::Success)
        {
            ++m_Version;
        }

        return result;
    }

    ECSResult EntityArchetype::CopyComponent(UInt32 entityID, ArchetypeChunk* chunk, const TypeID& typeID, void* destination)
    {
        return chunk->CopyComponent(entityID, typeID, destination);
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
