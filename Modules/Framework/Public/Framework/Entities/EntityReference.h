#pragma once
#include <Framework/Entities/EntityAssetRecords.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    //! @brief Non-owning serialized UUID reference; resolving never acquires asset residency.
    struct EntityReference final
    {
        //! @brief Authored or referenced UUID; never a runtime entity handle.
        Uuid m_uuid = Uuid::kNull;
        //! @brief Optional placement identity for diagnostics; does not acquire residency.
        IO::AssetID m_placementAsset = IO::AssetID::kNull;
        FE_RTTI_Reflect("84EE0DEE-3FE2-4BA6-A39D-3C24BB2E6C7B");

        //! @brief Resolve the current target generation without acquiring residency; inactive or unloaded targets may return null.
        [[nodiscard]] Entity* Resolve(const EntityWorld& world, bool activeOnly = true) const
        {
            return world.Find(m_uuid, activeOnly);
        }

        Serialization::ResultCode Serialize(Serialization::SerializationContext& context) const;
        Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context);
        static uint64_t RTTI_GetSerializationSchemaHash();
        static uint32_t RTTI_GetSerializationVersion();
    };
} // namespace FE::Framework
