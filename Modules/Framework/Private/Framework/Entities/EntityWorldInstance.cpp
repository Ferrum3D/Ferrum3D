#include <Framework/Entities/EntityWorldInstance.h>

namespace FE::Framework
{
    namespace
    {
        template<class T>
        bool CreateObjects(EntityWorld& world, festd::span<const Rtti::TypeID> types, festd::vector<Rtti::Any>& objects)
        {
            FE_Assert(objects.empty());
            for (const auto id : types)
            {
                const Rtti::Type* type = Rtti::TypeRegistry::FindType(id);
                if (!type || !type->m_defaultConstructor || !type->m_destructor || !type->m_cast)
                    return false;

                if (festd::find(type->m_baseTypes, T::TypeID) == type->m_baseTypes.end())
                    return false;

                const bool duplicate = std::any_of(objects.begin(), objects.end(), [id](const Rtti::Any& value) {
                    return value.GetType()->m_id == id;
                });
                if (duplicate)
                    return false;

                auto& value = objects.emplace_back();
                const bool constructed = value.Emplace(*type);
                FE_Assert(constructed);
                auto* object = static_cast<T*>(type->m_cast(value.GetValue(), T::TypeID));
                FE_Assert(object, "Reflected world object inheritance is inconsistent");
                if constexpr (std::same_as<T, WorldSystem>)
                    world.AddSystem(*object);
                else
                    world.AddService(*object);
            }

            return true;
        }
    } // namespace


    EntityWorldInstance::EntityWorldInstance(EntityAssetServices* assets)
        : m_world(assets)
    {
    }


    EntityWorldInstance::~EntityWorldInstance()
    {
        ClearObjects();
    }


    void EntityWorldInstance::ClearObjects()
    {
        // Destroy graphics membership and residency while the systems and their services still exist.
        m_world.Clear();
        for (uint32_t i = m_systems.size(); i > 0; --i)
        {
            auto& value = m_systems[i - 1];
            auto* system = static_cast<WorldSystem*>(value.GetType()->m_cast(value.GetValue(), WorldSystem::TypeID));
            m_world.RemoveSystem(*system);
        }

        m_systems.clear();
        for (uint32_t i = m_services.size(); i > 0; --i)
        {
            auto& value = m_services[i - 1];
            auto* service = static_cast<WorldService*>(value.GetType()->m_cast(value.GetValue(), WorldService::TypeID));
            m_world.RemoveService(*service);
        }

        m_services.clear();
        m_operations.clear();
    }


    bool EntityWorldInstance::Load(const EntityWorldAsset& definition)
    {
        if (!CreateObjects<WorldService>(m_world, definition.m_services, m_services)
            || !CreateObjects<WorldSystem>(m_world, definition.m_systems, m_systems))
        {
            ClearObjects();
            return false;
        }

        if (m_world.LoadDefinition(definition, &m_operations))
            return true;

        ClearObjects();
        return false;
    }


    bool EntityWorldInstance::Restore(const EntityWorldSnapshotAsset& snapshot)
    {
        if (!CreateObjects<WorldService>(m_world, snapshot.m_services, m_services)
            || !CreateObjects<WorldSystem>(m_world, snapshot.m_systems, m_systems))
        {
            ClearObjects();
            return false;
        }

        if (m_world.RestoreSnapshot(snapshot, &m_operations))
            return true;

        ClearObjects();
        return false;
    }


    MaterializationStatus EntityWorldInstance::GetStatus() const
    {
        MaterializationStatus result{ MaterializationState::kReady };
        for (const auto token : m_operations)
        {
            const auto status = m_world.GetMaterializationStatus(token);
            if (status.m_state == MaterializationState::kFailed || status.m_state == MaterializationState::kCanceled)
                return status;

            if (status.m_state == MaterializationState::kPending)
                result = status;
        }

        return result;
    }


    bool EntityWorldInstance::Capture(EntityWorldSnapshotAsset& snapshot) const
    {
        EntityWorldSnapshotAsset result;
        if (!m_world.CaptureSnapshot(result))
            return false;

        for (const auto& value : m_systems)
            result.m_systems.push_back(value.GetType()->m_id);
        for (const auto& value : m_services)
            result.m_services.push_back(value.GetType()->m_id);

        snapshot = std::move(result);
        return true;
    }
} // namespace FE::Framework
