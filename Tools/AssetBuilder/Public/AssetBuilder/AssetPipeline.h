#pragma once
#include <Core/IO/Path.h>
#include <Core/RTTI/RTTI.h>

namespace FE::AssetBuilder
{
    struct ImportAssetSettings final
    {
        IO::Path m_inputFile;
        IO::Path m_outputFile;
        Rtti::TypeID m_assetTypeId = Rtti::TypeID::kNull;
    };


    struct BuildAssetSettings final
    {
        IO::Path m_assetFile;
        IO::Path m_sourceRoot;
        IO::Path m_outputDirectory;
    };


    bool ImportAsset(const ImportAssetSettings& settings);
    bool BuildAsset(const BuildAssetSettings& settings);
} // namespace FE::AssetBuilder
