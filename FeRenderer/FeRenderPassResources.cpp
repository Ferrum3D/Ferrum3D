#include "FeRenderPassResources.h"
#include <FeCore/Console/FeLog.h>

namespace FE
{
    IFeRenderTarget* FeRenderPassResources::GetRenderTarget(const FeFrameGraphMutResource& resource)
    {
        return m_RenderTargets[resource.Index].get();
    }

    IFeShader* FeRenderPassResources::GetShader(const FeFrameGraphResource& resource)
    {
        return m_Shaders[resource.Index].get();
    }

    IFeTexture* FeRenderPassResources::GetTexture(const FeFrameGraphResource& resource)
    {
        return m_Textures[resource.Index].get();
    }

    FeFrameGraphMutResource FeRenderPassResources::CreateVirtualResource(FeFrameGraphResourceType type, const std::string& name)
    {
        FeFrameGraphMutResource resource{};
        resource.Index = m_Textures.size();
        resource.Name  = name;
        resource.Type  = type;

        switch (type)
        {
        case FeFrameGraphResourceType::Texture:
            m_Textures.emplace_back();
            break;
        case FeFrameGraphResourceType::Buffer:
            FE_ASSERT_MSG(false, "Buffers not implemented");
            break;
        case FeFrameGraphResourceType::Shader:
            m_Shaders.emplace_back();
            break;
        case FeFrameGraphResourceType::RenderTarget:
            m_RenderTargets.emplace_back();
            break;
        default:
            FE_ASSERT_MSG(false, "Invalid FeFrameGraphResourceType");
            break;
        }

        return resource;
    }

    void FeRenderPassResources::CreateRealResource(const FeFrameGraphResource& resource, std::shared_ptr<IFeTexture> texture)
    {
        FE_ASSERT(resource.Type == FeFrameGraphResourceType::Texture);
        m_Textures[resource.Index] = std::move(texture);
    }

    void FeRenderPassResources::CreateRealResource(const FeFrameGraphResource& resource, std::shared_ptr<IFeShader> shader)
    {
        FE_ASSERT(resource.Type == FeFrameGraphResourceType::Shader);
        m_Shaders[resource.Index] = std::move(shader);
    }

    void FeRenderPassResources::CreateRealResource(const FeFrameGraphResource& resource, std::shared_ptr<IFeRenderTarget> renderTarget)
    {
        FE_ASSERT(resource.Type == FeFrameGraphResourceType::RenderTarget);
        m_RenderTargets[resource.Index] = std::move(renderTarget);
    }
} // namespace FE
