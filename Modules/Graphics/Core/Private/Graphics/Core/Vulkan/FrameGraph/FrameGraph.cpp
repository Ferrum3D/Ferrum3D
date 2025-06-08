#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/Texture.h>
#include <Graphics/Core/Vulkan/Viewport.h>

namespace FE::Graphics::Vulkan
{
    FrameGraph::FrameGraph(Core::Device* device, Common::FrameGraphResourcePool* resourcePool, BindlessManager* bindlessManager)
        : Common::FrameGraph(device, resourcePool)
        , m_bindlessManager(bindlessManager)
    {
    }


    Core::ImageSRVDescriptor FrameGraph::GetSRV(Core::Texture* texture, Core::ImageSubresource subresource)
    {
        Texture* textureImpl = ImplCast(texture);

        FrameGraphContext* context = ImplCast(m_currentContext.Get());

        if (subresource == Core::ImageSubresource::kInvalid)
        {
            const Core::ImageDesc textureDesc = textureImpl->GetDesc();
            subresource.m_mostDetailedMipSlice = 0;
            subresource.m_mipSliceCount = textureDesc.m_mipSliceCount;
            subresource.m_firstArraySlice = 0;
            subresource.m_arraySize = textureDesc.m_arraySize;
        }

        const Core::ImageSubresourceIterator subresourceIterator{ subresource };
        for (const auto [mipIndex, arrayIndex] : subresourceIterator)
        {
            const TextureSubresourceState state = textureImpl->GetSubresourceState(mipIndex, arrayIndex);
            FE_Assert(state != TextureSubresourceState::kUndefined);

            if (state == TextureSubresourceState::kTransferDestination)
            {
                ImageBarrierDesc barrier;
                barrier.m_image = textureImpl->GetNative();
                barrier.m_subresource.m_mostDetailedMipSlice = mipIndex;
                barrier.m_subresource.m_mipSliceCount = 1;
                barrier.m_subresource.m_firstArraySlice = arrayIndex;
                barrier.m_subresource.m_arraySize = 1;
                barrier.m_sourceAccess = festd::to_underlying(Core::ImageWriteType::kTransferDestination);
                barrier.m_destAccess = festd::to_underlying(Core::ImageReadType::kShaderResource);
                barrier.m_sourceQueueKind = Core::HardwareQueueKindFlags::kTransfer;
                barrier.m_destQueueKind = Core::HardwareQueueKindFlags::kGraphics;
                context->m_resourceBarrierBatcher.AddBarrier(barrier);

                textureImpl->SetSubresourceState(mipIndex, arrayIndex, TextureSubresourceState::kShaderResource);
            }
        }

        const uint32_t descriptorIndex = m_bindlessManager->RegisterSRV(texture, subresource);
        return Core::ImageSRVDescriptor{ descriptorIndex };
    }


    Core::SamplerDescriptor FrameGraph::GetSampler(const Core::SamplerState sampler)
    {
        const uint32_t descriptorIndex = m_bindlessManager->RegisterSampler(sampler);
        return Core::SamplerDescriptor{ descriptorIndex };
    }


    void FrameGraph::PrepareExecute()
    {
        FE_PROFILER_ZONE();

        Viewport* viewport = ImplCast(m_viewport.Get());
        viewport->PrepareFrame();

        FrameGraphContext* context = Rc<FrameGraphContext>::New(&m_linearAllocator, m_device, this, m_bindlessManager);
        context->Init(ImplCast(m_viewport.Get())->GetCurrentGraphicsCommandBuffer());

        m_currentContext = context;
        m_bindlessManager->BeginFrame();
        context->m_resourceBarrierBatcher.Flush();
    }


    void FrameGraph::FinishExecute()
    {
        FE_PROFILER_ZONE();

        Viewport* viewport = ImplCast(m_viewport.Get());
        FrameGraphContext* context = ImplCast(m_currentContext.Get());

        context->m_resourceBarrierBatcher.Flush();
        context->EnqueueFenceToSignal(m_bindlessManager->CloseFrame());

        viewport->Present(context);
        m_currentContext.Reset();
    }


    void FrameGraph::FinishPassExecute(const PassData& pass)
    {
        (void)pass;
        ImplCast(m_currentContext.Get())->m_resourceBarrierBatcher.Flush();
    }
} // namespace FE::Graphics::Vulkan
