#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Strings/String.h>

namespace FE::Osmium
{
    class ShaderAssetStorage : public Assets::AssetStorage
    {
        friend class ShaderAssetLoader;

        String SourceCode;

        explicit ShaderAssetStorage(ShaderAssetLoader* loader);

    protected:
        void Delete() override;

    public:
        FE_CLASS_RTTI(ShaderAssetStorage, "AD125813-98AB-4960-A66C-80F5DEF4723C");

        [[nodiscard]] inline StringSlice GetSourceCode() const
        {
            return SourceCode;
        }
    };
} // namespace FE::Osmium
