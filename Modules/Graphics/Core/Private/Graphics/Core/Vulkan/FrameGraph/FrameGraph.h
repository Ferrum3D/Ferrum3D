#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>

namespace FE::Graphics::Vulkan
{
    struct GraphicsCommandQueue;
    struct BindlessManager;

    struct FrameGraph final : public Common::FrameGraph
    {
        FE_RTTI_Class(FrameGraph, "585305A0-06EB-4B16-8EF1-26FAACEB6AB8");

        FrameGraph(Core::Device* device, Common::FrameGraphResourcePool* resourcePool, BindlessManager* bindlessManager,
                   GraphicsCommandQueue* commandQueue);

        Core::ImageSRVDescriptor GetSRV(Core::Texture* texture, Core::ImageSubresource subresource) override;
        Core::ImageUAVDescriptor GetUAV(Core::RenderTarget* renderTarget, Core::ImageSubresource subresource) override;
        Core::BufferSRVDescriptor GetSRV(Core::Buffer* buffer) override;
        Core::BufferUAVDescriptor GetUAV(Core::Buffer* buffer) override;
        Core::SamplerDescriptor GetSampler(Core::SamplerState sampler) override;

    private:
        void PrepareSetup() override;
        void PrepareExecute() override;
        void FinishExecute() override;
        void PreparePassExecute(uint32_t passIndex) override;

        GraphicsCommandQueue* m_commandQueue = nullptr;
        BindlessManager* m_bindlessManager = nullptr;
    };
} // namespace FE::Graphics::Vulkan
