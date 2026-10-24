#pragma once
#include <Core/IO/AssetFile.h>

namespace FE::AssetBuilder
{
    using IO::AssetFile;
    using IO::AssetFileArtifact;
    using IO::AssetFileDependency;

    bool LoadAssetFile(const IO::Path& path, AssetFile& result);
    bool SaveAssetFile(const IO::Path& path, const AssetFile& assetFile);
} // namespace FE::AssetBuilder
