#include <AssetBuilder/AssetFile.h>
#include <Framework/Reflection.gen.h>
#include <GameFramework/Reflection.gen.h>
#include <Graphics/Reflection.gen.h>


namespace FE::AssetBuilder
{
    bool LoadAssetFile(const IO::Path& path, AssetFile& result)
    {
        // The file names types by UUID; retain complete module reflection before type-erased deserialization.
        CallLinkerAnchor_Framework();
        CallLinkerAnchor_GameFramework();
        CallLinkerAnchor_Graphics();

        return IO::LoadAssetDefinition(path, result);
    }


    bool SaveAssetFile(const IO::Path& path, const AssetFile& assetFile)
    {
        return IO::SaveAssetDefinition(path, assetFile);
    }
} // namespace FE::AssetBuilder
