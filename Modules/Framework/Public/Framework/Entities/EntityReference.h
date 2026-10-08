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
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000014");

        //! @brief Resolve the current target generation without acquiring residency; inactive or unloaded targets may return null.
        [[nodiscard]] Entity* Resolve(const EntityWorld& world, bool activeOnly = true) const
        {
            return world.Find(m_uuid, activeOnly);
        }

        FE_RTTI_Serialize();
        //! @brief Apply the deserialization UUID remap and clear placement metadata when ownership changes.
        void AfterDeserialize(Serialization::DeserializationContext& context);
    };
} // namespace FE::Framework
