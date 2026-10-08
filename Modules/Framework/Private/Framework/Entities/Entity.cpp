#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntityResources.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    Entity::Entity(EntityRegistry& registry, const EntityID id, const Uuid uuid, const Env::Name name)
        : m_uuid(uuid)
        , m_id(id)
        , m_name(name)
        , m_registry(&registry)
    {
    }


    Entity::~Entity()
    {
        FE_Assert(!m_resources || (m_resources->m_assets.empty() && m_resources->m_replacements.empty()));
        Memory::DefaultDelete(m_resources);
    }


    EntityWorld& Entity::GetWorld() const
    {
        return m_registry->GetWorld();
    }


    void* Entity::FindComponent(const Rtti::TypeID type, const bool write) const
    {
        return GetWorld().LookupComponent(*this, type, write);
    }
} // namespace FE::Framework
