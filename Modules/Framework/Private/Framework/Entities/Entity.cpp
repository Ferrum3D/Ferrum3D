#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    Entity::Entity(EntityWorld& world, EntityRegistry& registry, const EntityID id, const Uuid uuid, const Env::Name name)
        : m_world(&world)
        , m_registry(&registry)
        , m_id(id)
        , m_uuid(uuid)
        , m_name(name)
    {
        m_runtime = Memory::DefaultNew<Runtime>();
    }


    Entity::~Entity()
    {
        FE_Assert(m_runtime->m_assets.empty() && m_runtime->m_replacements.empty());
        Memory::DefaultDelete(m_runtime->m_residency);

        Memory::DefaultDelete(m_runtime);
    }


    void* Entity::FindComponent(const Rtti::TypeID type, const bool write) const
    {
        return m_world->LookupComponent(*this, type, write);
    }
} // namespace FE::Framework
