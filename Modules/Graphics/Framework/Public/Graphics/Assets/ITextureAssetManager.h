#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/Core/Image.h>

namespace FE::Graphics
{
    enum class AssetLoadingStatus : uint32_t
    {
        kNone,
        kLoading,
        kSucceeded,
        kFailed,
    };


    struct TextureAsset final : public Memory::RefCountedObjectBase
    {
        Env::Name m_name;
        Rc<Core::Image> m_resource;
        std::atomic<AssetLoadingStatus> m_status = AssetLoadingStatus::kNone;
    };


    struct ITextureAssetManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ITextureAssetManager, "DE919340-61E9-467E-96CB-CAF037A20A3A");

        virtual TextureAsset* Load(Env::Name assetName) = 0;
    };
} // namespace FE::Graphics
