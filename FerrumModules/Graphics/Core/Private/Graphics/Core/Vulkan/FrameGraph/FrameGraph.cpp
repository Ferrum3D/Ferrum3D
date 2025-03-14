#include <Graphics/Core/Vulkan/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/Viewport.h>

namespace FE::Graphics::Vulkan
{
    FrameGraph::FrameGraph(Core::Device* device, Common::FrameGraphResourcePool* resourcePool)
        : Common::FrameGraph(device, resourcePool)
    {
    }


    void FrameGraph::PrepareExecute()
    {
        FE_PROFILER_ZONE();

        Viewport* viewport = ImplCast(m_viewport.Get());
        viewport->PrepareFrame();

        FrameGraphContext* context = Rc<FrameGraphContext>::New(&m_linearAllocator, m_device, this);
        context->Init(ImplCast(m_viewport.Get())->GetCurrentGraphicsCommandBuffer());

        m_currentContext = context;
    }


    void FrameGraph::FinishExecute()
    {
        FE_PROFILER_ZONE();

        Viewport* viewport = ImplCast(m_viewport.Get());
        FrameGraphContext* context = ImplCast(m_currentContext.Get());

        viewport->Present(context);
        m_currentContext.Reset();
    }


    void FrameGraph::FinishPassExecute(const PassData& pass)
    {
        (void)pass;
        ImplCast(m_currentContext.Get())->m_resourceBarrierBatcher.Flush();
    }
} // namespace FE::Graphics::Vulkan
