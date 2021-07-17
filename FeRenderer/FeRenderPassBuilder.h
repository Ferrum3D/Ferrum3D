#pragma once
#include "FeFrameGraphResource.h"
#include "FeRenderAPI.h"
#include "FeRenderPassResources.h"
#include "IFeShader.h"
#include "IFeTexture.h"

namespace FE
{
    class FE_RENDER_API FeRenderPassBuilder
    {
        FeRenderPassResources* m_Resources;

    public:
        FeRenderPassBuilder(FeRenderPassResources* resources);

        FeFrameGraphMutResource Write(FeFrameGraphMutResource resource);

        FeFrameGraphMutResource Read(FeFrameGraphMutResource resource);
        FeFrameGraphResource Read(FeFrameGraphResource resource);

        FeFrameGraphResource CreateShader(const FeShaderLoadDesc& desc);
    };
} // namespace FE
