#pragma once
#include <Core/IO/Path.h>

namespace FE::AssetBuilder
{
    struct ImportAssetSettings final
    {
        IO::Path m_inputFile;
        IO::Path m_outputFile;
    };


    struct BuildAssetSettings final
    {
        IO::Path m_assetFile;
        IO::Path m_outputDirectory;
    };


    bool ImportAsset(const ImportAssetSettings& settings);
    bool BuildAsset(const BuildAssetSettings& settings);
} // namespace FE::AssetBuilder
