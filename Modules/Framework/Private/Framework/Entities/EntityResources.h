#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    struct EntityResources final
    {
        struct AssetContribution
        {
            Rtti::TypeID m_component;
            IO::AssetID m_asset;
            Rtti::TypeID m_expectedType;
            uint64_t m_transition = 0;
        };

        struct Replacement
        {
            const EntityComponentInfo* m_info;
            void* m_data;
            uint64_t m_transition;
            ComponentStage m_stage = ComponentStage::kNone;
        };

        EntityResidencySet m_residency;
        festd::vector<Replacement> m_replacements;
        festd::vector<AssetContribution> m_assets;

        explicit EntityResources(EntityAssetServices& services)
            : m_residency(services)
        {
        }
    };
} // namespace FE::Framework
