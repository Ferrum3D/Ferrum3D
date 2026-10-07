#pragma once
#include <AssetBuilder/AssetPipeline.h>
#include <Framework/Entities/EntityCollection.h>

namespace FE::AssetBuilder
{
    bool ImportEntityCollection(const IO::Path& output, const Framework::EntityCollection& collection);
    bool ImportEntityPlacement(const IO::Path& output, const Framework::EntityCollectionInstanceAsset& placement,
                               const Framework::EntityCollection& collection);
} // namespace FE::AssetBuilder
