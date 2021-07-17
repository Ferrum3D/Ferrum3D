#include "FeRenderPassBuilder.h"

namespace FE
{
    FeRenderPassBuilder::FeRenderPassBuilder(FeRenderPassResources* resources)
    {
        m_Resources = resources;
    }

    FeFrameGraphMutResource FeRenderPassBuilder::Write(FeFrameGraphMutResource resource)
    {
        auto res = resource;
        res.Version++;
        return res;
    }

    FeFrameGraphMutResource FeRenderPassBuilder::Read(FeFrameGraphMutResource resource)
    {
        return resource;
    }

    FeFrameGraphResource FeRenderPassBuilder::Read(FeFrameGraphResource resource)
    {
        return resource;
    }

    FeFrameGraphResource FeRenderPassBuilder::CreateShader(const FeShaderLoadDesc& desc)
    {
        return m_Resources->CreateVirtualResource(FeFrameGraphResourceType::Shader, desc.Name);
    }
} // namespace FE
