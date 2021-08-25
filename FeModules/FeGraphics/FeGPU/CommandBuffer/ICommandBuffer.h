#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/Object.h>
#include <FeGPU/Resource/ResourceState.h>
#include <FeGPU/Descriptors/IDescriptorTable.h>
#include <FeGPU/Pipeline/IGraphicsPipeline.h>
#include <cstdint>

namespace FE::GPU
{
    class ICommandBuffer : public IObject
    {
    public:
        ~ICommandBuffer() override = default;

        FE_CLASS_RTTI(ICommandBuffer, "80A845FD-5E8F-4BF1-BB75-880DE377D4A2");

        virtual void Begin() = 0;
        virtual void End()   = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor)    = 0;

        virtual void ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers) = 0;
        virtual void MemoryBarrier()                                                                   = 0;

        virtual void BindDescriptorTables(const Vector<IDescriptorTable*>& descriptorTables) = 0;

        virtual void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) = 0;
        virtual void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) = 0;
    };
} // namespace FE::GPU
