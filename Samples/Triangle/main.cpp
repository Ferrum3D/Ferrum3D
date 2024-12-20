#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Framework/ApplicationModule.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <OsGPU/OsmiumGPU.h>
#include <OsGPU/OsmiumGPUModule.h>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
    [[maybe_unused]] FE::Float32 RGB[3];
};

namespace HAL = FE::Osmium;

inline constexpr const char* ExampleName = "Ferrum3D - Triangle";

class TestApplication final : public FE::ApplicationModule
{
    FE::Rc<RHI::IInstance> m_Instance;
    FE::Rc<RHI::IAdapter> m_Adapter;
    FE::Rc<RHI::IDevice> m_Device;

    FE::List<FE::Rc<RHI::IFence>> m_Fences;
    FE::List<FE::Rc<RHI::Framebuffer>> m_Framebuffers;
    FE::List<FE::Rc<RHI::ICommandBuffer>> m_CommandBuffers;
    FE::Rc<RHI::ICommandQueue> m_GraphicsQueue;
    FE::Rc<RHI::ICommandQueue> m_TransferQueue;

    FE::Rc<RHI::RenderPass> m_RenderPass;
    FE::Rc<RHI::Swapchain> m_SwapChain;
    FE::Rc<RHI::GraphicsPipeline> m_Pipeline;
    FE::List<RHI::IImageView*> m_RTVs;

    FE::Rc<RHI::ShaderModule> m_PixelShader;
    FE::Rc<RHI::ShaderModule> m_VertexShader;

    FE::Rc<RHI::IBuffer> m_VertexBuffer;

    FE::Rc<RHI::IWindow> m_Window;
    RHI::Viewport m_Viewport{};
    RHI::Scissor m_Scissor{};

    const FE::int32_t m_FrameBufferCount = 3;

protected:
    void PollSystemEvents() override
    {
        m_Window->PollEvents();
    }

    bool CloseEventReceived() override
    {
        if (m_Window)
        {
            return m_Window->CloseRequested();
        }

        return false;
    }

    void Tick(const FE::FrameEventArgs& /* frameEventArgs */) override
    {
        auto frameIndex = m_SwapChain->GetCurrentFrameIndex();

        m_Fences[frameIndex]->WaitOnCPU();
        m_Window->PollEvents();
        auto imageIndex = m_SwapChain->GetCurrentImageIndex();
        m_Fences[m_SwapChain->GetCurrentFrameIndex()]->Reset();
        m_GraphicsQueue->SubmitBuffers(
            { m_CommandBuffers[imageIndex].Get() }, m_Fences[frameIndex].Get(), RHI::SubmitFlags::FrameBeginEnd);
        m_SwapChain->Present();
    }

    void GetFrameworkDependencies(FE::List<FE::Rc<FE::IModuleFactory>>& dependencies) override
    {
        dependencies.Push(RHI::OsmiumGPUModule::CreateFactory());
    }

public:
    FE_RTTI_Class(TestApplication, "1DF20010-27B5-4B68-943A-FBE881DF24F4");

    ~TestApplication() override
    {
        m_Device->WaitIdle();
    }

    void Initialize(const FE::ApplicationDesc& desc) override
    {
        ApplicationModule::Initialize(desc);
        auto module = FE::ServiceLocator<RHI::OsmiumGPUModule>::Get();
        module->Initialize(RHI::OsmiumGPUModuleDesc(ExampleName, RHI::GraphicsAPI::Vulkan));

        m_Instance      = module->CreateInstance();
        m_Adapter       = m_Instance->GetAdapters()[0];
        m_Device        = m_Adapter->CreateDevice();
        m_GraphicsQueue = m_Device->GetCommandQueue(RHI::CommandQueueClass::Graphics);
        m_Window        = m_Device->CreateWindow(RHI::WindowDesc{ Desc.WindowWidth, Desc.WindowHeight, ExampleName });
        m_Viewport      = m_Window->CreateViewport();
        m_Scissor       = m_Window->CreateScissor();

        RHI::SwapchainDesc swapChainDesc{};
        swapChainDesc.ImageCount         = m_FrameBufferCount;
        swapChainDesc.ImageWidth         = m_Scissor.Width();
        swapChainDesc.ImageHeight        = m_Scissor.Height();
        swapChainDesc.NativeWindowHandle = m_Window->GetNativeHandle();
        swapChainDesc.Queue              = m_GraphicsQueue.Get();
        swapChainDesc.VerticalSync       = false;
        m_SwapChain                      = m_Device->CreateSwapChain(swapChainDesc);
        {
            // clang-format off
            FE::List<Vertex> vertexData = {
                {{+0.0, -0.5, 0}, {1, 0, 0}},
                {{+0.5, +0.5, 0}, {0, 1, 0}},
                {{-0.5, +0.5, 0}, {0, 0, 1}}
            };
            // clang-format on

            RHI::BufferDesc bufferDesc{};
            bufferDesc.Flags = RHI::BindFlags::VertexBuffer;
            bufferDesc.Size  = vertexData.Size() * sizeof(Vertex);
            m_VertexBuffer   = m_Device->CreateBuffer(bufferDesc);
            m_VertexBuffer->AllocateMemory(RHI::MemoryType::kHostVisible);
            m_VertexBuffer->UpdateData(vertexData.Data());
        }

        auto compiler = m_Device->CreateShaderCompiler();
        RHI::ShaderCompilerArgs shaderArgs{};
        shaderArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
        shaderArgs.EntryPoint = "main";

        shaderArgs.Stage      = RHI::ShaderStage::kPixel;
        shaderArgs.FullPath   = "../../Samples/Triangle/Shaders/PixelShader.hlsl";
        auto source           = FE::IO::File::ReadAllText(shaderArgs.FullPath);
        shaderArgs.SourceCode = source;
        auto psByteCode       = compiler->CompileShader(shaderArgs);

        shaderArgs.Stage      = RHI::ShaderStage::kVertex;
        shaderArgs.FullPath   = "../../Samples/Triangle/Shaders/VertexShader.hlsl";
        source                = FE::IO::File::ReadAllText(shaderArgs.FullPath);
        shaderArgs.SourceCode = source;
        auto vsByteCode       = compiler->CompileShader(shaderArgs);
        compiler.Reset();

        m_PixelShader  = m_Device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kPixel, psByteCode));
        m_VertexShader = m_Device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kVertex, vsByteCode));

        RHI::RenderPassDesc renderPassDesc{};

        RHI::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format       = m_SwapChain->GetDesc().Format;
        attachmentDesc.InitialState = RHI::ResourceState::kUndefined;
        attachmentDesc.FinalState   = RHI::ResourceState::kPresent;
        renderPassDesc.Attachments  = { attachmentDesc };

        RHI::SubpassDesc subpassDesc{};
        subpassDesc.RenderTargetAttachments = { RHI::SubpassAttachment(RHI::ResourceState::kRenderTarget, 0) };
        renderPassDesc.Subpasses            = { subpassDesc };
        RHI::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = { dependency };
        m_RenderPass                       = m_Device->CreateRenderPass(renderPassDesc);

        RHI::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = RHI::InputLayoutBuilder{}
                                       .AddBuffer(RHI::InputStreamRate::kPerVertex)
                                       .AddAttribute(RHI::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(RHI::Format::R32G32B32_SFloat, "COLOR")
                                       .Build()
                                       .Build();

        FE::List shaders{ m_PixelShader.Get(), m_VertexShader.Get() };

        pipelineDesc.RenderPass             = m_RenderPass.Get();
        pipelineDesc.SubpassIndex           = 0;
        pipelineDesc.ColorBlend             = RHI::ColorBlendState({ RHI::TargetColorBlending{} });
        pipelineDesc.Shaders                = shaders;
        pipelineDesc.Rasterization          = RHI::RasterizationState{};
        pipelineDesc.Rasterization.CullMode = RHI::CullingModeFlags::kBack;
        pipelineDesc.Scissor                = m_Scissor;
        pipelineDesc.Viewport               = m_Viewport;

        m_Pipeline = m_Device->CreateGraphicsPipeline(pipelineDesc);

        for (FE::size_t i = 0; i < m_SwapChain->GetDesc().FrameCount; ++i)
        {
            m_Fences.Push(m_Device->CreateFence(RHI::FenceState::Signaled));
        }

        m_RTVs = m_SwapChain->GetRTVs();
        for (FE::size_t i = 0; i < m_SwapChain->GetImageCount(); ++i)
        {
            RHI::FramebufferDesc framebufferDesc{};
            framebufferDesc.RenderPass        = m_RenderPass.Get();
            framebufferDesc.RenderTargetViews = { m_RTVs[i] };
            framebufferDesc.Width             = m_Scissor.Width();
            framebufferDesc.Height            = m_Scissor.Height();
            auto framebuffer                  = m_Framebuffers.Push(m_Device->CreateFramebuffer(framebufferDesc));

            auto& cmd = m_CommandBuffers.Push(m_Device->CreateCommandBuffer(RHI::CommandQueueClass::Graphics));
            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.Get());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_VertexBuffer.Get(), 0);
            cmd->BeginRenderPass(m_RenderPass.Get(),
                                 framebuffer.Get(),
                                 { RHI::ClearValueDesc::CreateColorValue(FE::Colors::MediumAquamarine) });
            cmd->Draw(3, 1, 0, 0);
            cmd->EndRenderPass();
            cmd->End();
        }
    }
};

FE_APP_MAIN()
{
    auto app = FE::MakeShared<TestApplication>();
    app->Initialize(FE::ApplicationDesc(ExampleName));
    return app->RunMainLoop();
}
