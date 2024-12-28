#pragma once
#include <FeCore/Containers/HashTables.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>

namespace FE::ECS
{
    union Entity final
    {
        struct
        {
            uint32_t m_chunkID : 20;
            uint32_t m_entityID : 12;
        };

        uint32_t m_value;
    };


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

        void FreeIndex(const uint32_t index)
        {
            m_freeIndices[index] = true;
        }
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
        uint32_t m_entityCount;

        Entity CreateEntity(festd::vector<const ArchetypeChunk*>& globalChunkTable);
        void DestroyEntity(Entity entity, festd::vector<const ArchetypeChunk*>& globalChunkTable);

        explicit Archetype(festd::span<const uint32_t> componentSizes);
        ~Archetype() = default;

        Archetype(const Archetype&) = delete;
        Archetype(Archetype&&) = delete;
        Archetype& operator=(const Archetype&) = delete;
        Archetype& operator=(Archetype&&) = delete;
    };


    struct ArchetypeChunk final
    {
        IndexAllocator m_indexAllocator;
        Archetype* m_archetype = nullptr;
        uint32_t m_chunkID = kInvalidIndex;
        uint32_t m_allocatedEntityCount = 0;

        void* GetComponentPtr(const uint32_t index, const ArchetypeComponentSpec& componentSpec)
        {
            auto* address = reinterpret_cast<uint8_t*>(this) + sizeof(*this);
            auto* arrayAddress = address + componentSpec.m_chunkOffset;
            return arrayAddress + static_cast<size_t>(componentSpec.m_size) * index;
        }

        uint16_t* GetIndexLookupTable(const uint32_t entitySize, const uint32_t entityCount)
        {
            auto* address = reinterpret_cast<uint8_t*>(this) + sizeof(*this);
            return reinterpret_cast<uint16_t*>(address + entitySize * entityCount);
        }

        static constexpr size_t kChunkSize = 4096;

        static uint32_t CalculateEntityCount(const uint32_t entitySize)
        {
            const uint32_t archetypeSize = AlignUp<uint32_t>(sizeof(ArchetypeChunk), Memory::kDefaultAlignment);
            return static_cast<uint32_t>((kChunkSize - archetypeSize) / (entitySize + sizeof(uint16_t)));
        }

        static ArchetypeChunk* New()
        {
            void* ptr = Memory::DefaultAllocate(kChunkSize);
            return new (ptr) ArchetypeChunk();
        }

        static void Delete(ArchetypeChunk* chunk)
        {
            chunk->~ArchetypeChunk();
            Memory::DefaultFree(chunk);
        }
    };


    struct EntityRegistry final : public Memory::RefCountedObjectBase
    {
        festd::vector<const ArchetypeChunk*> m_globalChunkTable;
        festd::unordered_dense_map<uint64_t, Rc<Archetype>> m_archetypes;

        template<class T, class... TArgs>
        T& AddComponent(Entity& entity, TArgs&&... args)
        {
            void* componentPtr = Memory::DefaultAllocate(sizeof(T));
            entity = {};

            return *new (componentPtr) T(std::forward<TArgs>(args)...);
        }

        template<class T>
        T& GetComponent(Entity entity)
        {
            void* componentPtr = nullptr;
            return *static_cast<T*>(componentPtr);
        }

        template<class... TComponents>
        Entity CreateEntity()
        {
            uint64_t archetypeID = GenerateArchetypeID<TComponents...>();
            constexpr uint32_t componentSizes[] = { sizeof(TComponents)... };
            Entity entity;

            if (m_archetypes.find(archetypeID) != m_archetypes.end())
            {
                entity = m_archetypes[archetypeID]->CreateEntity(m_globalChunkTable);
            }
            else
            {
                m_archetypes[archetypeID] = Rc<Archetype>::DefaultNew(componentSizes);
                entity = m_archetypes[archetypeID]->CreateEntity(m_globalChunkTable);
            }

            (AddComponent<TComponents>(entity), ...);

            return entity;
        }

    private:
        template<class TFirstComponent, class... TComponents>
        static constexpr uint64_t GenerateArchetypeID()
        {
            uint64_t typeID = TypeNameHash<TFirstComponent>;
            (HashCombine(typeID, TypeNameHash<TComponents>), ...);
            return typeID;
        }
    };
} // namespace FE::ECS
