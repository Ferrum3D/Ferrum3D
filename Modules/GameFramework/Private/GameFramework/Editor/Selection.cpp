#include <GameFramework/Editor/Selection.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Graphics/Scene/RenderViewData.h>
#include <Shaders/Editor/Selection.h>

namespace FE::Editor
{
    SelectionViewModule::SelectionViewModule(Graphics::View* view)
        : ViewModuleBase(view)
    {
    }


    void SelectionViewModule::Initialize(Graphics::Core::PipelineFactory* factory)
    {
        Graphics::Core::ComputePipelineRequest request;
        request.m_desc.SetComputeShader("Shaders/Editor/Selection.cs.hlsl");
        m_pipeline = factory->CreateComputePipeline(request);
        m_pipeline->GetCompletionWaitGroup()->Wait();
        FE_Assert(m_pipeline->IsReady(), "Editor selection shader failed to compile");
    }


    void SelectionViewModule::Update(Graphics::Core::FrameGraphBlackboard&) {}


    void SelectionViewModule::SetSelection(const festd::span<const uint32_t> objects)
    {
        m_selected.assign(objects.begin(), objects.end());
    }


    void SelectionViewModule::RequestPick(const Vector2UInt pixel)
    {
        if (m_pickPending)
            return;

        m_pickPosition = pixel;
        m_pickRequested = true;
    }


    bool SelectionViewModule::ConsumePick(uint32_t& object)
    {
        if (!m_pickPending || !m_pickFence.IsReady())
            return false;

        const auto* mapped = static_cast<const uint32_t*>(m_readback->Map());
        m_readback->InvalidateMappedRange(0, sizeof(uint32_t));
        object = *mapped;
        m_readback->Unmap();
        m_pickPending = false;
        return true;
    }


    void SelectionViewModule::AddRenderPasses(Graphics::Core::FrameGraph& graph, Graphics::Core::FrameGraphBlackboard& blackboard)
    {
        using namespace Graphics;

        const auto* visibility = blackboard.TryGet<Visibility::Output>();
        if (!m_pipeline || !visibility || (m_selected.empty() && !m_pickRequested))
            return;

        const auto& view = blackboard.Get<RendererViewData>();
        const Vector2UInt resolution = view.m_mainColorTarget->GetDesc().GetSize2D();
        auto selected =
            Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), "EditorSelection", Math::Max(1u, m_selected.size()));
        if (!m_selected.empty())
        {
            FE_Verify(
                view.m_renderQueueUploader
                    ->UploadGenerated(graph, selected.Get(), m_selected.size() * sizeof(uint32_t), [this](void* destination) {
                        memcpy(destination, m_selected.data(), m_selected.size() * sizeof(uint32_t));
                    }));
        }
        else
        {
            FE_Verify(view.m_renderQueueUploader->Upload(graph, selected.Get(), uint32_t{ 0 }));
        }

        auto result = Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), "EditorPick", 1);
        auto* pass = graph.AllocatePassData<SelectionPassDesc>();
        pass->m_pipeline = m_pipeline;
        pass->m_constants.m_visibility = graph.GetSRV(visibility->m_target.Get());
        pass->m_constants.m_color = graph.GetUAV(view.m_mainColorTarget);
        pass->m_constants.m_selected = graph.GetSRV(selected.Get());
        pass->m_constants.m_pickResult = graph.GetUAV(result.Get());
        pass->m_constants.m_resolution = resolution;
        pass->m_constants.m_selectedCount = m_selected.size();
        pass->m_constants.m_pickPosition = Math::Min(m_pickPosition, resolution - Vector2UInt(1, 1));
        pass->m_constants.m_pick = m_pickRequested ? 1 : 0;
        graph.AddDispatchPass("EditorSelectionOutlineAndPick", pass, Math::CeilDivide(resolution, 8u));

        if (!m_pickRequested)
            return;

        auto& renderer = Renderer::Get();
        if (!m_readback)
        {
            m_readback = Core::Buffer::CreateByteAddress(graph.GetDevice(), "EditorPickReadback", sizeof(uint32_t));
            Core::ResourceCommitParams commit;
            commit.m_memory = Core::ResourceMemory::kHostRandomAccess;
            commit.m_bindFlags = Core::BarrierAccessFlags::kCopyDest;
            renderer.GetResourcePool()->CommitBufferMemory(m_readback.Get(), commit);
        }

        graph.AddCopyPass(Core::BufferView(m_readback.Get()), Core::BufferView(result.Get()));
        m_pickFence = renderer.GetGraphicsQueue()->GetCurrentFence();
        m_pickRequested = false;
        m_pickPending = true;
    }


    void SelectionViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Editor
