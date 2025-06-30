#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>
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
            subresource = Core::ImageSubresource::CreateWhole(textureDesc);
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
                barrier.m_sourceAccess = Core::ImageAccessType::kTransferDestination;
                barrier.m_destAccess = Core::ImageAccessType::kShaderResource;
                barrier.m_sourceQueueKind = Core::HardwareQueueKindFlags::kTransfer;
                barrier.m_destQueueKind = Core::HardwareQueueKindFlags::kGraphics;
                context->m_resourceBarrierBatcher.AddBarrier(barrier);

                textureImpl->SetSubresourceState(mipIndex, arrayIndex, TextureSubresourceState::kShaderResource);
            }
        }

        const uint32_t descriptorIndex = m_bindlessManager->RegisterSRV(texture, subresource);
        return Core::ImageSRVDescriptor{ descriptorIndex };
    }


    Core::ImageUAVDescriptor FrameGraph::GetUAV(Core::RenderTarget* renderTarget, Core::ImageSubresource subresource)
    {
        const uint32_t descriptorIndex = m_bindlessManager->RegisterUAV(renderTarget, subresource);
        return Core::ImageUAVDescriptor{ descriptorIndex };
    }


    Core::BufferSRVDescriptor FrameGraph::GetSRV(Core::Buffer* buffer)
    {
        const uint32_t descriptorIndex = m_bindlessManager->RegisterSRV(buffer);
        return Core::BufferSRVDescriptor{ descriptorIndex };
    }


    Core::BufferUAVDescriptor FrameGraph::GetUAV(Core::Buffer* buffer)
    {
        const uint32_t descriptorIndex = m_bindlessManager->RegisterUAV(buffer);
        return Core::BufferUAVDescriptor{ descriptorIndex };
    }


    Core::SamplerDescriptor FrameGraph::GetSampler(const Core::SamplerState sampler)
    {
        const uint32_t descriptorIndex = m_bindlessManager->RegisterSampler(sampler);
        return Core::SamplerDescriptor{ descriptorIndex };
    }


    void FrameGraph::PrepareSetup()
    {
        FE_PROFILER_ZONE();

        Viewport* viewport = ImplCast(m_viewport.Get());
        viewport->PrepareFrame();
    }


    void FrameGraph::PrepareExecute()
    {
        FE_PROFILER_ZONE();

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


    void FrameGraph::PreparePassExecute(const uint32_t passIndex)
    {
        FrameGraphContext* context = ImplCast(m_currentContext.Get());
        auto& barriers = context->m_resourceBarrierBatcher;
        auto& pass = m_passes[passIndex];

        ResourceAccess* access = pass.m_accessesListHead;
        while (access)
        {
            auto& resource = m_resources[access->m_resourceIndex];

            if (resource.m_accessState == access->m_flags)
            {
                access = access->m_next;
                continue;
            }

            switch (resource.m_resourceType)
            {
            default:
            case Core::ResourceType::kTexture:
            case Core::ResourceType::kUnknown:
                FE_DebugBreak();
                [[fallthrough]];

            case Core::ResourceType::kBuffer:
                {
                    const auto* buffer = fe_assert_cast<Buffer*>(resource.m_resource.Get());

                    BufferBarrierDesc barrier;
                    barrier.m_buffer = buffer->GetNative();
                    barrier.m_sourceAccess = static_cast<Core::BufferAccessType>(resource.m_accessState);
                    barrier.m_destAccess = static_cast<Core::BufferAccessType>(access->m_flags);
                    barrier.m_sourceQueueKind = Core::HardwareQueueKindFlags::kGraphics;
                    barrier.m_destQueueKind = Core::HardwareQueueKindFlags::kGraphics;

                    barriers.AddBarrier(barrier);
                }
                break;

            case Core::ResourceType::kRenderTarget:
                {
                    const auto* renderTarget = fe_assert_cast<RenderTarget*>(resource.m_resource.Get());

                    ImageBarrierDesc barrier;
                    barrier.m_image = renderTarget->GetNative();
                    barrier.m_subresource = Core::ImageSubresource::CreateWhole(renderTarget->GetDesc());
                    barrier.m_sourceAccess = static_cast<Core::ImageAccessType>(resource.m_accessState);
                    barrier.m_destAccess = static_cast<Core::ImageAccessType>(access->m_flags);
                    barrier.m_sourceQueueKind = Core::HardwareQueueKindFlags::kGraphics;
                    barrier.m_destQueueKind = Core::HardwareQueueKindFlags::kGraphics;

                    barriers.AddBarrier(barrier);
                }
                break;
            }

            resource.m_accessState = access->m_flags;
            access = access->m_next;
        }

        barriers.Flush();
    }
} // namespace FE::Graphics::Vulkan
