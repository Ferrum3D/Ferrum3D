#pragma once
#include <Framework/Entities/EntityAssetRecords.h>
#include <Framework/Entities/EntityWorld.h>

namespace FE::Framework
{
    struct EntityReference final
    {
        FE_SERIALIZE_NAME("uuid") Uuid m_uuid = Uuid::kNull;
        FE_SERIALIZE_NAME("placementAsset") IO::AssetID m_placementAsset = IO::AssetID::kNull;
        FE_RTTI_Reflect("b9dbe80d-6ab0-486a-ab00-000000000014");
        [[nodiscard]] Entity* Resolve(const EntityWorld& world, bool activeOnly = true) const
        {
            return world.Find(m_uuid, activeOnly);
        }
        FE_RTTI_Serialize();
        void AfterDeserialize(Serialization::DeserializationContext& context);
    };
} // namespace FE::Framework
