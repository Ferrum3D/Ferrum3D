#pragma once
#include <FeCore/Memory/Memory.h>
#include <FrameGraph/Resource/FrameGraphResource.h>

namespace FE::FG
{
    class FrameGraphBuilder
    {
    public:
        FE_CLASS_RTTI(FrameGraphBuilder, "71E90E47-A00C-4600-B487-75AE04D4ABAB");

        FrameGraphResource Read(FrameGraphResource resource);
        FrameGraphMutResource Write(FrameGraphMutResource resource);
        FrameGraphMutResource UseRenderTarget(FrameGraphMutResource resource, UInt32 textureIndex);
    };
}
