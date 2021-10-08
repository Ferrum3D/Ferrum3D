#pragma once
#include <FeCore/Memory/Memory.h>
#include <FrameGraph/Resource/IFrameGraphResource.h>

namespace FE::FG
{
    class IRenderPass;

    class IFrameGraph : public IObject
    {
    protected:
        virtual void AddResource(IFrameGraphResource* resource)                        = 0;
        virtual void ReadResource(IFrameGraphResource* resource, IRenderPass* reader)  = 0;
        virtual void WriteResource(IFrameGraphResource* resource, IRenderPass* writer) = 0;

    public:
        FE_CLASS_RTTI(IFrameGraph, "C0A24175-0C5D-4C97-9D84-51EC54024FB8");

        ~IFrameGraph() override = default;

        virtual void AddRenderPass(IRenderPass* renderPass)                     = 0;
        virtual void AddRetainedResource(IFrameGraphResource* retainedResource) = 0;
        virtual void Compile()                                                  = 0;
        virtual void Execute()                                                  = 0;
    };
} // namespace FE::FG
