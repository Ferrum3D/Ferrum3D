#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/GraphicsCommandQueue.h>

namespace FE::Graphics::Vulkan
{
    FrameGraph::FrameGraph(Core::Device* device, Core::DescriptorManager* descriptorManager, Core::ResourcePool* resourcePool,
                           GraphicsCommandQueue* commandQueue)
        : Common::FrameGraph(device, descriptorManager, resourcePool)
        , m_commandQueue(commandQueue)
    {
    }


    FrameGraph::~FrameGraph() = default;


    void FrameGraph::PrepareExecuteInternal()
    {
        FE_PROFILER_ZONE();

        FrameGraphContext* context = Rc<FrameGraphContext>::New(&m_linearAllocator, m_device, this, m_descriptorManager);
        context->Init(m_commandQueue->GetCurrentGraphicsCommandBuffer());
        m_currentContext = context;
    }


    void FrameGraph::FinishExecuteInternal()
    {
        FE_PROFILER_ZONE();

        FrameGraphContext* context = ImplCast(m_currentContext.Get());
        context->EnqueueFenceToSignal(m_descriptorManager->CloseFrame());

        m_currentContext.Reset();
    }
} // namespace FE::Graphics::Vulkan
