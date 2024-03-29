#include <FeCore/Assets/IAssetManager.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT UInt32 MeshAssetStorage_VertexSize(MeshAssetStorage* self)
        {
            return self->VertexSize();
        }

        FE_DLL_EXPORT UInt32 MeshAssetStorage_IndexSize(MeshAssetStorage* self)
        {
            return self->IndexSize();
        }

        FE_DLL_EXPORT const Float32* MeshAssetStorage_VertexData(MeshAssetStorage* self)
        {
            return self->VertexData();
        }

        FE_DLL_EXPORT const UInt32* MeshAssetStorage_IndexData(MeshAssetStorage* self)
        {
            return self->IndexData();
        }
    }
} // namespace FE::Osmium
