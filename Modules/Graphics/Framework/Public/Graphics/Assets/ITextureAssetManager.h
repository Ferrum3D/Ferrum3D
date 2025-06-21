#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics
{
    enum class AssetLoadingStatus : uint32_t
    {
        kNone,
        kHasLoadedMips,
        kCompletelyLoaded,
        kFailed,
    };


    struct TextureAsset final : public Memory::RefCountedObjectBase
    {
        Env::Name m_name;
        Rc<Core::Texture> m_resource;
        std::atomic<AssetLoadingStatus> m_status = AssetLoadingStatus::kNone;
        Rc<WaitGroup> m_completionWaitGroup;
    };


    struct ITextureAssetManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ITextureAssetManager, "DE919340-61E9-467E-96CB-CAF037A20A3A");

        virtual TextureAsset* Load(Env::Name assetName) = 0;
    };
} // namespace FE::Graphics
