#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::FG
{
    class IFrameGraph : public IObject
    {
    public:
        FE_CLASS_RTTI(IFrameGraph, "C0A24175-0C5D-4C97-9D84-51EC54024FB8");

        // virtual void CreateTexture(UInt64 id, const FrameGraphTextureDesc& desc) = 0;
        // virtual Shared<ITexture> GetTexture(FrameGraphResource resource) = 0;

        virtual void Compile() = 0;
        virtual void Execute() = 0;
    };
} // namespace FE::FG
