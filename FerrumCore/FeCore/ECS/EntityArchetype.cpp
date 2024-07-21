#include <EASTL/sort.h>
#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    EntityArchetype::EntityArchetype(const ArraySlice<ComponentType>& layout)
    {
        m_Layout.assign(layout.Data(), layout.Data() + layout.Length());
    }

    void EntityArchetype::InitInternal()
    {
        eastl::sort(m_Layout.begin(), m_Layout.end(), [](const ComponentType& lhs, const ComponentType& rhs) {
            if (lhs.Alignment == rhs.Alignment)
            {
                return eastl::hash<TypeID>{}(lhs.Type) > eastl::hash<TypeID>{}(rhs.Type);
            }

            return lhs.Alignment > rhs.Alignment;
        });

        for (uint32_t i = 0; i < m_Layout.size(); ++i)
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
        m_Chunks.push_back(ArchetypeChunk::Allocate());
        m_Chunks.back()->Init(desc);
        return m_Chunks.back();
    }

    void EntityArchetype::DeallocateChunk(ArchetypeChunk* chunk)
    {
        auto it = eastl::find(m_Chunks.begin(), m_Chunks.end(), chunk);
        FE_ASSERT_MSG(it != m_Chunks.end(), "Couldn't find ArchetypeChunk: {}", chunk);
        (*it)->Deallocate();
        m_Chunks.erase_unsorted(it);
    }

    void EntityArchetype::CollectGarbage()
    {
        eastl::vector<ArchetypeChunk*> newChunks;
        for (ArchetypeChunk*& chunk : m_Chunks)
        {
            if (chunk->Empty())
            {
                chunk->Deallocate();
            }
            else
            {
                newChunks.push_back(chunk);
            }
        }

        m_Chunks = std::move(newChunks);
    }

    ECSResult EntityArchetype::CreateEntity(UInt16& id, ArchetypeChunk** chunk)
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

    ECSResult EntityArchetype::DestroyEntity(UInt16 id, ArchetypeChunk* chunk)
    {
        auto result = chunk->DeallocateEntity(id);
        if (result == ECSResult::Success)
        {
            ++m_Version;
        }

        return result;
    }

    ECSResult EntityArchetype::UpdateComponent(UInt16 entityID, ArchetypeChunk* chunk, const TypeID& typeID, const void* source)
    {
        auto result = chunk->UpdateComponent(entityID, typeID, source);
        if (result == ECSResult::Success)
        {
            ++m_Version;
        }

        return result;
    }

    ECSResult EntityArchetype::CopyComponent(UInt16 entityID, ArchetypeChunk* chunk, const TypeID& typeID, void* destination)
    {
        return chunk->CopyComponent(entityID, typeID, destination);
    }

    EntityArchetypeMatch EntityArchetype::Match(const EntityArchetype& other)
    {
        USize matchCount = 0;
        for (uint32_t i = 0; i < ComponentTypeCount(); ++i)
        {
            for (uint32_t j = 0; j < other.ComponentTypeCount(); ++j)
            {
                if (m_Layout[i].Type == other.m_Layout[j].Type)
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

        for (uint32_t i = 0; i < lhs.ComponentTypeCount(); ++i)
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
