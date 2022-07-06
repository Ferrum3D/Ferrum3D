#pragma once
#include <FeCore/Framework/ModuleFramework.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>
#include <OsAssets/Images/ImageAssetStorage.h>

namespace FE::Osmium
{
    struct OsmiumAssetsModuleDesc
    {
    };

    class OsmiumAssetsModule : public ModuleFramework<OsmiumAssetsModule>
    {
    protected:
        void GetFrameworkDependencies(List<Shared<IFrameworkFactory>>& dependencies) override;

    public:
        ~OsmiumAssetsModule() override = default;

        inline static constexpr const char* LibraryPath = "OsAssets";

        virtual void Initialize(const OsmiumAssetsModuleDesc& desc) = 0;
    };
}
