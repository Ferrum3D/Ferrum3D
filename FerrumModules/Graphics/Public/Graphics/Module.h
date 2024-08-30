#pragma once
#include <FeCore/Framework/ModuleBase.h>
#include <OsAssets/Images/ImageAssetStorage.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>
#include <OsAssets/Shaders/ShaderAssetStorage.h>

namespace FE::Osmium
{
    class OsmiumAssetsModule : public ModuleBase
    {
    public:
        ~OsmiumAssetsModule() override = default;

        FE_RTTI_Class(OsmiumAssetsModule, "1746E16A-F5EF-4AD0-A91D-541CA8D5F2E8");

        inline static constexpr const char* LibraryPath = "OsAssets";
    };
} // namespace FE::Osmium
