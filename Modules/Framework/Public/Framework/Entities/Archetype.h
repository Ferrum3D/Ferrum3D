#pragma once
#include <Framework/Entities/Base.h>
#include <festd/vector.h>

namespace FE::Framework
{
    struct ArchetypeComponentDesc final
    {
        uint32_t m_byteSize;
        uint32_t m_byteOffset;
    };


    struct EntityAllocationResult final
    {
        ArchetypeChunk* m_chunk = nullptr;
        uint32_t m_entityIndex = kInvalidIndex;

        static const EntityAllocationResult kInvalid;
    };

    inline const EntityAllocationResult EntityAllocationResult::kInvalid{ nullptr, kInvalidIndex };


    struct Archetype final
    {
        EntityRegistry* m_registry = nullptr;
        festd::inline_vector<ArchetypeChunk*> m_chunks;
        festd::vector<const EntityComponentInfo*> m_componentTypes;
        festd::vector<ComponentTypeID> m_componentTypeIDs;
        festd::vector<ArchetypeComponentDesc> m_components;

        uint32_t m_entityByteSize = 0;

        static Archetype* Create(EntityRegistry* registry, festd::span<const ComponentTypeID> componentTypes);
        static void Destroy(const Archetype* archetype);

        EntityAllocationResult AllocateEntity();

        Archetype(EntityRegistry* registry, festd::span<const ComponentTypeID> componentTypes);
        ~Archetype();

        [[nodiscard]] bool MatchesAll(festd::span<const ComponentTypeID> includedComponentTypes) const;

        [[nodiscard]] bool MatchesAny(festd::span<const ComponentTypeID> includedComponentTypes) const;

        template<class... TComponents>
        [[nodiscard]] bool MatchesAll() const
        {
            constexpr ComponentTypeID includedComponentTypes[] = { ComponentTypeID::Create<TComponents>()... };
            return MatchesAll(includedComponentTypes);
        }

        template<class... TComponents>
        [[nodiscard]] bool MatchesAny() const
        {
            constexpr ComponentTypeID includedComponentTypes[] = { ComponentTypeID::Create<TComponents>()... };
            return MatchesAny(includedComponentTypes);
        }

    private:
        static constexpr uint32_t kInitialChunkSize = 4096;
        uint32_t m_chunkByteSize = kInitialChunkSize;
    };


    struct ArchetypeChunk final
    {
        Archetype* m_archetype;
        uint32_t m_worldID : 8;
        uint32_t m_chunkID : 24;
        uint32_t m_entityCount;
        uint32_t m_byteSize;
        uint16_t* m_indexLookupTable;
        std::byte* m_data;
        uint64_t* m_allocatedEntitiesBitSet;

        static ArchetypeChunk* Create();

        ArchetypeChunk() = default;

        void Setup(uint32_t chunkID, Archetype* archetype, uint32_t byteSize);

        [[nodiscard]] FE_FORCE_INLINE void* GetComponentData(const uint32_t entityIndex, const uint32_t componentIndex) const
        {
            const ArchetypeComponentDesc componentDesc = m_archetype->m_components[componentIndex];
            const uint32_t realIndex = m_indexLookupTable[entityIndex];
            return m_data //
                + static_cast<size_t>(componentDesc.m_byteOffset) * m_entityCount
                + static_cast<size_t>(componentDesc.m_byteSize) * realIndex;
        }

        [[nodiscard]] FE_FORCE_INLINE void* GetComponentArray(const uint32_t componentIndex) const
        {
            const ArchetypeComponentDesc componentDesc = m_archetype->m_components[componentIndex];
            return m_data + static_cast<size_t>(componentDesc.m_byteOffset) * m_entityCount;
        }

        template<class TComponent>
        [[nodiscard]] TComponent* SafeGetComponentArray() const
        {
            constexpr ComponentTypeID componentTypeID = ComponentTypeID::Create<TComponent>();
            const uint32_t componentIndex = festd::find_index(m_archetype->m_componentTypeIDs, componentTypeID);
            if (componentIndex == kInvalidIndex)
                return nullptr;

            return static_cast<TComponent*>(GetComponentArray(componentIndex));
        }

        [[nodiscard]] uint32_t Allocate() const;
        void Free(uint32_t entityIndex) const;
    };
} // namespace FE::Framework
