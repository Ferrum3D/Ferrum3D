#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/Barrier.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>


namespace FE::Graphics::Vulkan
{
    FrameGraph::FrameGraph(Core::Device* device, Core::DescriptorManager* descriptorManager, Core::ResourcePool* resourcePool,
                           Core::GraphicsQueue* commandQueue)
        : Common::FrameGraph(device, descriptorManager, resourcePool)
        , m_commandQueue(commandQueue)
    {
    }


    FrameGraph::~FrameGraph() = default;


    void FrameGraph::BeginFrame()
    {
        m_descriptorManager->BeginFrame(m_commandQueue->GetCurrentFence());
    }


    void FrameGraph::PrepareExecuteInternal()
    {
        FE_PROFILER_ZONE();

        auto* commandQueue = Rtti::AssertCast<GraphicsQueue*>(m_commandQueue);
        CommandBuffer* commandBuffer = commandQueue->GetCurrentCommandBuffer();

        auto* context = Memory::DefaultNew<FrameGraphContext>(m_device, this, m_descriptorManager);
        context->Init(commandBuffer);
        m_currentContext = context;
    }


    void FrameGraph::FinishExecuteInternal()
    {
        FE_PROFILER_ZONE();

        m_descriptorManager->EndFrame();

        m_currentContext.Reset();
    }


    void FrameGraph::ExecutePassBarriersInternal(PassNode& pass)
    {
        FE_PROFILER_ZONE();

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<VkImageMemoryBarrier2> imageBarriers{ &temp };
        festd::pmr::vector<VkBufferMemoryBarrier2> bufferBarriers{ &temp };

        auto* commandQueue = Rtti::AssertCast<GraphicsQueue*>(m_commandQueue);
        CommandBuffer* commandBuffer = commandQueue->GetCurrentCommandBuffer();
        const VkCommandBuffer vkCommandBuffer = commandBuffer->GetNative();

        for (const Core::FenceSyncPoint& wait : pass.m_ownershipTransferWaits)
            commandBuffer->EnqueueFenceToWait(wait);

        const auto flushBarriers = [&]() {
            if (imageBarriers.empty() && bufferBarriers.empty())
                return;

            VkDependencyInfo dependencyInfo = {};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.imageMemoryBarrierCount = imageBarriers.size();
            dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
            dependencyInfo.bufferMemoryBarrierCount = bufferBarriers.size();
            dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
            vkCmdPipelineBarrier2(vkCommandBuffer, &dependencyInfo);

            imageBarriers.clear();
            bufferBarriers.clear();
        };

        for (const Core::TextureBarrierDesc& barrier : pass.m_textureOwnershipTransferBarriers)
            imageBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        for (const Core::BufferBarrierDesc& barrier : pass.m_bufferOwnershipTransferBarriers)
            bufferBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        flushBarriers();

        for (const Core::TextureBarrierDesc& barrier : pass.m_texturePostOwnershipBarriers)
            imageBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        for (const Core::TextureBarrierDesc& barrier : pass.m_barrierBatcher.m_textureBarriers)
            imageBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        for (const Core::BufferBarrierDesc& barrier : pass.m_barrierBatcher.m_bufferBarriers)
            bufferBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        flushBarriers();
    }
} // namespace FE::Graphics::Vulkan
