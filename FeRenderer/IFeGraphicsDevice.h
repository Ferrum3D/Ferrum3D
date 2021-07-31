#pragma once
#include "IFeShader.h"
#include "IFeTexture.h"
#include <FeCore/Assets/AssetManager.h>

namespace FE
{
    enum class FeRenderBackend
    {
        None,
        Direct3D11,
        Direct3D12,
        Vulkan
    };

    struct FeGraphicsDeviceDesc
    {
        FeRenderBackend Backend{ FeRenderBackend::Direct3D12 };
    };

    class IFeGraphicsDevice
    {
    public:
        virtual std::shared_ptr<IFeShader> CreateShader(const FeShaderLoadDesc& desc, const std::string& sourceCode) = 0;
        virtual std::shared_ptr<IFeTexture> CreateTexture(const TextureLoadDesc& desc, RawAsset imageAsset)          = 0;
    };
} // namespace FE
