#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/RTTI/RTTI.h>
#include <festd/vector.h>

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

        //! Optional: receives the products whose artifact was published, as opposed to found up to date.
        festd::vector<IO::AssetID>* m_publishedProducts = nullptr;
    };


    bool ImportAsset(const ImportAssetSettings& settings);
    bool BuildAsset(const BuildAssetSettings& settings);
} // namespace FE::AssetBuilder
