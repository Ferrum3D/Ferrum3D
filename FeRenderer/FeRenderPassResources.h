#pragma once
#include "FeFrameGraphResource.h"
#include "FeRenderAPI.h"
#include "IFeRenderTarget.h"
#include "IFeShader.h"
#include "IFeTexture.h"
#include <vector>

namespace FE
{
    class FE_RENDER_API FeRenderPassResources
    {
        std::vector<std::shared_ptr<IFeTexture>> m_Textures;
        std::vector<std::shared_ptr<IFeShader>> m_Shaders;
        std::vector<std::shared_ptr<IFeRenderTarget>> m_RenderTargets;

    public:
        IFeRenderTarget* GetRenderTarget(const FeFrameGraphMutResource& resource);
        IFeShader* GetShader(const FeFrameGraphResource& resource);
        IFeTexture* GetTexture(const FeFrameGraphResource& resource);

        FeFrameGraphMutResource CreateVirtualResource(FeFrameGraphResourceType type, const std::string& name);

        void CreateRealResource(const FeFrameGraphResource& resource, std::shared_ptr<IFeTexture> texture);
        void CreateRealResource(const FeFrameGraphResource& resource, std::shared_ptr<IFeShader> shader);
        void CreateRealResource(const FeFrameGraphResource& resource, std::shared_ptr<IFeRenderTarget> renderTarget);
    };
} // namespace FE
