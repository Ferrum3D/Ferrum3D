#pragma once
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>

namespace FE::ECS
{
    union Entity
    {
        struct
        {
            uint32_t m_chunkID : 20;
            uint32_t m_entityID : 12;
        };

        uint32_t m_value;
    };

    struct ArchetypeChunk;

    struct ArchetypeComponentSpec final
    {
        uint32_t m_size;
        uint32_t m_chunkOffset;
    };

    struct Archetype final : public Memory::RefCountedObjectBase
    {
        festd::vector<ArchetypeChunk*> m_chunks;
        festd::vector<ArchetypeComponentSpec> m_componentSpecs;
        uint32_t m_entitySize;

        Entity CreateEntity();
        void DestroyEntity(Entity entity);

        Archetype(festd::span<uint32_t> componentSizes);
    };

    template<class... TComponents>
    Entity CreateEntity();

    template<class T, class... TArgs>
    T& AddComponent(Entity& entity, TArgs&&... args)
    {
        void* componentPtr = nullptr;
        entity = {};
        return *new(componentPtr) T(std::forward<TArgs>(args)...);
    }

    template<class T>
    T& GetComponent(Entity entity)
    {
        void* componentPtr = nullptr;
        return *static_cast<T*>(componentPtr);
    }

    struct IndexAllocator final
    {
        festd::vector<bool> m_freeIndices;

        uint32_t AllocateIndex()
        {
            for (uint32_t i = 0; i < m_freeIndices.size(); ++i)
            {
                if (m_freeIndices[i])
                {
                    m_freeIndices[i] = false;
                    return i;
                }
            }

            m_freeIndices.push_back(false);
            return m_freeIndices.size() - 1;
        }

        void FreeIndex(uint32_t index)
        {
            m_freeIndices[index] = true;
        }
    };

    struct ArchetypeChunk final
    {
        IndexAllocator m_indexAllocator;
        Archetype* m_archetype;
        uint32_t m_chunkID;

        void* GetComponentPtr(uint32_t index, const ArchetypeComponentSpec& componentSpec)
        {
            auto* address = reinterpret_cast<uint8_t*>(this) + sizeof(*this);
            auto* arrayAddress = address + componentSpec.m_chunkOffset;
            return arrayAddress + static_cast<size_t>(componentSpec.m_size) * index;
        }

        uint16_t* GetIndexLookupTable(uint32_t entitySize, uint32_t entityCount)
        {
            auto* address = reinterpret_cast<uint8_t*>(this) + sizeof(*this);
            return reinterpret_cast<uint16_t*>(address + entitySize * entityCount);
        }

        static constexpr size_t kChunkSize = 4096;

        static uint32_t CalculateEntityCount(uint32_t entitySize)
        {
            const uint32_t archetypeSize = AlignUp<uint32_t>(sizeof(ArchetypeChunk), Memory::kDefaultAlignment);
            return static_cast<uint32_t>((kChunkSize - archetypeSize) / (entitySize + sizeof(uint16_t)));
        }

        static ArchetypeChunk* New()
        {
            void* ptr = Memory::DefaultAllocate(kChunkSize);
            return new(ptr) ArchetypeChunk();
        }

        static void Delete(ArchetypeChunk* chunk)
        {
            chunk->~ArchetypeChunk();
            Memory::DefaultFree(chunk);
        }
    };
} // namespace FE::ECS
