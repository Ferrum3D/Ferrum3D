#include <FeCore/Memory/FiberTempAllocator.h>
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


    void FrameGraph::PrepareExecuteInternal()
    {
        FE_PROFILER_ZONE();

        auto* commandQueue = Rtti::AssertCast<GraphicsQueue*>(m_commandQueue);
        CommandBuffer* commandBuffer = commandQueue->GetCurrentCommandBuffer();

        FrameGraphContext* context = Rc<FrameGraphContext>::New(&m_linearAllocator, m_device, this, m_descriptorManager);
        context->Init(commandBuffer);
        m_currentContext = context;
    }


    void FrameGraph::FinishExecuteInternal()
    {
        FE_PROFILER_ZONE();

        FrameGraphContext* context = ImplCast(m_currentContext.Get());
        context->EnqueueFenceToSignal(m_descriptorManager->CloseFrame());

        m_currentContext.Reset();
    }


    void FrameGraph::ExecutePassBarriersInternal(PassNode& pass)
    {
        FE_PROFILER_ZONE();

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<VkImageMemoryBarrier2> imageBarriers{ &temp };
        festd::pmr::vector<VkBufferMemoryBarrier2> bufferBarriers{ &temp };

        for (const Core::TextureBarrierDesc& barrier : pass.m_textureOwnershipTransferBarriers)
            imageBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        for (const Core::BufferBarrierDesc& barrier : pass.m_bufferOwnershipTransferBarriers)
            bufferBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        for (const Core::TextureBarrierDesc& barrier : pass.m_barrierBatcher.m_textureBarriers)
            imageBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        for (const Core::BufferBarrierDesc& barrier : pass.m_barrierBatcher.m_bufferBarriers)
            bufferBarriers.push_back(TranslateBarrier(barrier, ImplCast(m_device)));

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = imageBarriers.size();
        dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
        dependencyInfo.bufferMemoryBarrierCount = bufferBarriers.size();
        dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();

        auto* commandQueue = Rtti::AssertCast<GraphicsQueue*>(m_commandQueue);
        const CommandBuffer* commandBuffer = commandQueue->GetCurrentCommandBuffer();
        const VkCommandBuffer vkCommandBuffer = commandBuffer->GetNative();
        vkCmdPipelineBarrier2(vkCommandBuffer, &dependencyInfo);
    }
} // namespace FE::Graphics::Vulkan
