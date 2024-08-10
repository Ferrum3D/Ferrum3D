#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/ECS/ComponentType.h>
#include <FeCore/Memory/RefCount.h>

namespace FE::ECS
{
    class ArchetypeChunk;

    enum class EntityArchetypeMatch
    {
        None, //!< None of the components match.
        Some, //!< Some components are present.
        All   //!< All components are present.
    };

    //! \brief Describes an entity archetype - a collection of component types and associated component data pools.
    //!
    //! Entity archetype is a unique combination of component types (see \ref ComponentType). A component is contained
    //! within a particular archetype if and only if it has exactly the same set of components. For example, if we
    //! have these entities:
    //!
    //! ```
    //! Entity0 { Translation, Rotation, Renderer, Physics }
    //! Entity1 { Translation, Rotation, Renderer }
    //! Entity2 { Translation, Rotation, Renderer }
    //! Entity3 { Translation, Renderer, Physics }
    //! ```
    //!
    //! Only Entity1 and Entity2 will have the same archetype. Entity0 and Entity3 will have dedicated archetypes.
    //! But all this entities have some components in common, so we can get all of them from a system using entity queries.
    //! For example here is visualization of call `Query<Translation, Rotation>()`:
    //!
    //! ```
    //!     Entity0            Entity1       Entity2            Entity3
    //!        |                  |             |                  |
    //!        v                  v             v                  v
    //! +-------------+    +-------------+-------------+    +-------------+
    //! | Translation | <<<| Translation | Translation | <<<| Translation | <<<
    //! |  Rotation   | <<<|  Rotation   |  Rotation   | <<<|             | <xx
    //! |  Renderer   |    |  Renderer   |  Renderer   |    |  Renderer   |
    //! |   Physics   |    |             |             |    |   Physics   |
    //! +-------------+    +-------------+-------------+    +-------------+
    //!   Archetype0                Archetype1                Archetype2
    //! ```
    //!
    //! Here `<xx` shows that the Rotation component was not assigned to the Entity3. That's why the query will return only
    //! these entities: Entity0, Entity1, Entity2.
    //!
    //! The archetypes are built automatically by the Entity Component System when an entity with previously unknown
    //! archetype is created.
    class EntityArchetype final
    {
        friend class EntityArchetypeBuilder;
        friend class EntityQuery;

        inline static constexpr uint32_t ChunkByteSize = 16 * 1024;

        eastl::vector<ComponentType> m_Layout;
        uint32_t m_EntitySize = 0;
        size_t m_HashCode = 0;

        eastl::vector<ArchetypeChunk*> m_Chunks;

        uint32_t m_Version = 0;

        void InitInternal();

        //! \brief Create a new entity archetype from a list of component types.
        explicit EntityArchetype(const ArraySlice<ComponentType>& layout);

    public:
        FE_RTTI_Base(EntityArchetype, "EC825348-557C-4728-A44B-9CB7FCADA938");

        inline EntityArchetype() = default;
        ~EntityArchetype();

        inline EntityArchetype(const EntityArchetype& other) = default;

        inline EntityArchetype(EntityArchetype&& other) noexcept
        {
            m_Layout = std::move(other.m_Layout);
            m_EntitySize = other.m_EntitySize;
            m_HashCode = other.m_HashCode;
            m_Chunks = std::move(other.m_Chunks);
            m_Version = other.m_Version;
        }

        ArchetypeChunk* AllocateChunk();
        void DeallocateChunk(ArchetypeChunk* chunk);

        inline ArraySlice<ArchetypeChunk*> Chunks()
        {
            return m_Chunks;
        }

        //! \brief Deallocate all unused chunks.
        void CollectGarbage();

        //! \brief Create entity that contains all of archetype's components.
        //!
        //! \note This function will only reserve the space, contents are left undefined!
        //!
        //! \param [out] id    - The ID of the created entity local to the archetype chunk.
        //! \param [out] chunk - The chunk where the entity was allocated.
        ECSResult CreateEntity(uint16_t& id, ArchetypeChunk** chunk);

        //! \brief Destroy the entity.
        //!
        //! The specified id-chunk pair must be previously returned from this archetype via CreateEntity() function.
        //! The memory previously used for the components of this entity will be undefined. It can be reused by the ECS to store
        //! new entities.
        //!
        //! \param [in] id    - The ID of the entity to destroy local to the archetype chunk.
        //! \param [in] chunk - The chunk where the entity was allocated.
        ECSResult DestroyEntity(uint16_t id, ArchetypeChunk* chunk);

        //! \brief Retrieve the pointer to the component to set its data.
        //!
        //! \param [in] entityID - The ID of the entity local to the archetype chunk.
        //! \param [in] chunk    - The chunk where the entity was allocated.
        //! \param [in] typeID   - The ID of component type.
        //! \param [in] source   - The data of component to be copied.
        ECSResult UpdateComponent(uint16_t entityID, ArchetypeChunk* chunk, const TypeID& typeID, const void* source);

        //! \brief Retrieve the pointer to the component to get its data.
        //!
        //! \param [in] entityID    - The ID of the entity local to the archetype chunk.
        //! \param [in] chunk       - The chunk where the entity was allocated.
        //! \param [in] typeID      - The ID of component type.
        //! \param [in] destination - The buffer to copy the component data to.
        ECSResult CopyComponent(uint16_t entityID, ArchetypeChunk* chunk, const TypeID& typeID, void* destination);

        //! \brief Get the version of the archetype - a number that gets incremented after every change.
        [[nodiscard]] inline uint32_t Version() const
        {
            return m_Version;
        }

        //! \brief Check if the archetype has been changed after the specified version. Same as `version != archetype.Version()`
        [[nodiscard]] inline bool DidChange(uint32_t version) const
        {
            return m_Version != version;
        }

        //! \brief Get sum of sizes of all component types in the archetype.
        [[nodiscard]] size_t EntitySize() const
        {
            return m_EntitySize;
        }

        //! \brief Get the number of entities of this archetype that can be stored in a single ArchetypeChunk.
        [[nodiscard]] uint32_t ChunkCapacity() const
        {
            return ChunkByteSize / m_EntitySize;
        }

        //! \brief Get number of chunks currently allocated for this archetype.
        [[nodiscard]] size_t ChunkCount() const
        {
            return m_Chunks.size();
        }

        //! \brief Get number of component types in the archetype.
        [[nodiscard]] size_t ComponentTypeCount() const
        {
            return m_Layout.size();
        }

        //! \brief Get component types in the archetype.
        [[nodiscard]] ArraySlice<ComponentType> ComponentTypes() const
        {
            return m_Layout;
        }

        //! \brief Get combination of all component type hashes from the archetype.
        [[nodiscard]] size_t GetHash() const
        {
            return m_HashCode;
        }

        //! \brief Match the archetype against another one.
        //!
        //! \param [in] other - The other archetype.
        //!
        //! \see EntityArchetypeMatch
        EntityArchetypeMatch Match(const EntityArchetype& other);

        friend bool operator==(const EntityArchetype& lhs, const EntityArchetype& rhs);

        inline friend bool operator!=(const EntityArchetype& lhs, const EntityArchetype& rhs)
        {
            return !(rhs == lhs);
        }
    };

    //! \brief This class helps building instances of EntityArchetype.
    //!
    //! Example usage:
    //! ```cpp
    //! auto archetype = EntityArchetypeBuilder{}
    //!     .AddComponent<TranslationComponent>()
    //!     .AddComponent<RotationComponent>()
    //!     .Build();
    //! ```
    class EntityArchetypeBuilder
    {
        EntityArchetype m_Archetype;

    public:
        inline EntityArchetypeBuilder() = default;

        inline explicit EntityArchetypeBuilder(const ArraySlice<ComponentType>& componentTypes)
            : m_Archetype(componentTypes)
        {
        }

        //! \brief Add a component type to EntityArchetype.
        inline EntityArchetypeBuilder& AddComponentType(const ComponentType& componentType)
        {
            m_Archetype.m_Layout.push_back(componentType);
            return *this;
        }

        //! \brief Add a component type to EntityArchetype.
        template<class T>
        inline EntityArchetypeBuilder& AddComponentType()
        {
            m_Archetype.m_Layout.push_back(ComponentType::Create<T>());
            return *this;
        }

        //! \brief Build entity archetype.
        inline EntityArchetype Build()
        {
            m_Archetype.InitInternal();
            return m_Archetype;
        }
    };
} // namespace FE::ECS

template<>
struct eastl::hash<FE::ECS::EntityArchetype>
{
    inline size_t operator()(const FE::ECS::EntityArchetype& value) const noexcept
    {
        return value.GetHash();
    }
};
