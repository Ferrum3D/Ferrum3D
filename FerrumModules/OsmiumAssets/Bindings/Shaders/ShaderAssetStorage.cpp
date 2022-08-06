#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <OsAssets/Shaders/ShaderAssetStorage.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void ShaderAssetStorage_GetSourceCode(ShaderAssetStorage* self, ByteBuffer* result)
        {
            auto code = self->GetSourceCode();
            *result   = ByteBuffer::CopyString(StringSlice(code.Data(), code.Size()));
        }
    }
} // namespace FE::Osmium
