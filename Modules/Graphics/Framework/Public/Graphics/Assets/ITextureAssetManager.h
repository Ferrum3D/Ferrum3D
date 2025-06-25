#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/Assets/Base.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics
{
    struct TextureAsset final : public Asset
    {
        Rc<Core::Texture> m_resource;
    };


    struct ITextureAssetManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(ITextureAssetManager, "DE919340-61E9-467E-96CB-CAF037A20A3A");

        virtual TextureAsset* Load(Env::Name assetName) = 0;
    };
} // namespace FE::Graphics
