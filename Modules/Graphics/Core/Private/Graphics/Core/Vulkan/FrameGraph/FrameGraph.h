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

        ImageSRVDescriptor GetSRV(const Core::Texture* texture, Core::ImageSubresource subresource) override;
        ImageSRVDescriptor GetSRV(const Core::RenderTarget* texture, Core::ImageSubresource subresource) override;
        ImageUAVDescriptor GetUAV(const Core::RenderTarget* renderTarget, Core::ImageSubresource subresource) override;
        BufferSRVDescriptor GetSRV(const Core::Buffer* buffer, uint32_t offset, uint32_t size) override;
        BufferUAVDescriptor GetUAV(const Core::Buffer* buffer, uint32_t offset, uint32_t size) override;
        SamplerDescriptor GetSampler(Core::SamplerState sampler) override;

    private:
        void PrepareSetup() override;
        void PrepareExecute() override;
        void FinishExecute() override;
        void PreparePassExecute(uint32_t passIndex) override;

        GraphicsCommandQueue* m_commandQueue = nullptr;
        BindlessManager* m_bindlessManager = nullptr;
    };
} // namespace FE::Graphics::Vulkan
