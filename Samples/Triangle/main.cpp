#include <FeCore/Containers/IByteBuffer.h>
#include <FeCore/Framework/ApplicationFramework.h>
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

class TestApplication final : public FE::ApplicationFramework
{
    FE::Shared<HAL::IInstance> m_Instance;
    FE::Shared<HAL::IAdapter> m_Adapter;
    FE::Shared<HAL::IDevice> m_Device;

    FE::List<FE::Shared<HAL::IFence>> m_Fences;
    FE::List<FE::Shared<HAL::IFramebuffer>> m_Framebuffers;
    FE::List<FE::Shared<HAL::ICommandBuffer>> m_CommandBuffers;
    FE::Shared<HAL::ICommandQueue> m_GraphicsQueue;
    FE::Shared<HAL::ICommandQueue> m_TransferQueue;

    FE::Shared<HAL::IRenderPass> m_RenderPass;
    FE::Shared<HAL::ISwapChain> m_SwapChain;
    FE::Shared<HAL::IGraphicsPipeline> m_Pipeline;
    FE::List<FE::Shared<HAL::IImageView>> m_RTVs;

    FE::Shared<HAL::IShaderModule> m_PixelShader;
    FE::Shared<HAL::IShaderModule> m_VertexShader;

    FE::Shared<HAL::IBuffer> m_VertexBuffer;

    FE::Shared<HAL::IWindow> m_Window;
    HAL::Viewport m_Viewport{};
    HAL::Scissor m_Scissor{};

    const FE::Int32 m_FrameBufferCount = 3;

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
            { m_CommandBuffers[imageIndex].GetRaw() }, m_Fences[frameIndex], HAL::SubmitFlags::FrameBeginEnd);
        m_SwapChain->Present();
    }

    void GetFrameworkDependencies(FE::List<FE::Shared<FE::IFrameworkFactory>>& dependencies) override
    {
        dependencies.Push(HAL::OsmiumGPUModule::CreateFactory());
    }

public:
    FE_CLASS_RTTI(TestApplication, "1DF20010-27B5-4B68-943A-FBE881DF24F4");

    ~TestApplication() override
    {
        m_Device->WaitIdle();
    }

    void Initialize(const FE::ApplicationDesc& desc) override
    {
        ApplicationFramework::Initialize(desc);
        auto module = FE::SharedInterface<HAL::OsmiumGPUModule>::Get();
        module->Initialize(HAL::OsmiumGPUModuleDesc(ExampleName, HAL::GraphicsAPI::Vulkan));

        m_Instance      = module->CreateInstance();
        m_Adapter       = m_Instance->GetAdapters()[0];
        m_Device        = m_Adapter->CreateDevice();
        m_GraphicsQueue = m_Device->GetCommandQueue(HAL::CommandQueueClass::Graphics);
        m_Window        = m_Device->CreateWindow(HAL::WindowDesc{ Desc.WindowWidth, Desc.WindowHeight, ExampleName });
        m_Viewport      = m_Window->CreateViewport();
        m_Scissor       = m_Window->CreateScissor();

        HAL::SwapChainDesc swapChainDesc{};
        swapChainDesc.ImageCount         = m_FrameBufferCount;
        swapChainDesc.ImageWidth         = m_Scissor.Width();
        swapChainDesc.ImageHeight        = m_Scissor.Height();
        swapChainDesc.NativeWindowHandle = m_Window->GetNativeHandle();
        swapChainDesc.Queue              = m_GraphicsQueue.GetRaw();
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
            m_VertexBuffer = m_Device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexData.Size() * sizeof(Vertex));
            m_VertexBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
            m_VertexBuffer->UpdateData(vertexData.Data());
        }

        auto compiler = m_Device->CreateShaderCompiler();
        HAL::ShaderCompilerArgs shaderArgs{};
        shaderArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        shaderArgs.EntryPoint = "main";

        shaderArgs.Stage      = HAL::ShaderStage::Pixel;
        shaderArgs.FullPath   = "../../Samples/Triangle/Shaders/PixelShader.hlsl";
        auto source           = FE::IO::File::ReadAllText(shaderArgs.FullPath);
        shaderArgs.SourceCode = source;
        auto psByteCode       = compiler->CompileShader(shaderArgs);

        shaderArgs.Stage      = HAL::ShaderStage::Vertex;
        shaderArgs.FullPath   = "../../Samples/Triangle/Shaders/VertexShader.hlsl";
        source                = FE::IO::File::ReadAllText(shaderArgs.FullPath);
        shaderArgs.SourceCode = source;
        auto vsByteCode       = compiler->CompileShader(shaderArgs);
        compiler.Reset();

        m_PixelShader  = m_Device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));
        m_VertexShader = m_Device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Vertex, vsByteCode));

        HAL::RenderPassDesc renderPassDesc{};

        HAL::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format       = m_SwapChain->GetDesc().Format;
        attachmentDesc.InitialState = HAL::ResourceState::Undefined;
        attachmentDesc.FinalState   = HAL::ResourceState::Present;
        renderPassDesc.Attachments  = { attachmentDesc };

        HAL::SubpassDesc subpassDesc{};
        subpassDesc.RenderTargetAttachments = { HAL::SubpassAttachment(HAL::ResourceState::RenderTarget, 0) };
        renderPassDesc.Subpasses            = { subpassDesc };
        HAL::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = { dependency };
        m_RenderPass                       = m_Device->CreateRenderPass(renderPassDesc);

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder{}
                                       .AddBuffer(HAL::InputStreamRate::PerVertex)
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "COLOR")
                                       .Build()
                                       .Build();

        pipelineDesc.RenderPass             = m_RenderPass;
        pipelineDesc.SubpassIndex           = 0;
        pipelineDesc.ColorBlend             = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
        pipelineDesc.Shaders                = { m_PixelShader, m_VertexShader };
        pipelineDesc.Rasterization          = HAL::RasterizationState{};
        pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;
        pipelineDesc.Scissor                = m_Scissor;
        pipelineDesc.Viewport               = m_Viewport;

        m_Pipeline = m_Device->CreateGraphicsPipeline(pipelineDesc);

        for (FE::USize i = 0; i < m_SwapChain->GetDesc().FrameCount; ++i)
            m_Fences.Push(m_Device->CreateFence(HAL::FenceState::Signaled));

        m_RTVs = m_SwapChain->GetRTVs();
        for (FE::USize i = 0; i < m_SwapChain->GetImageCount(); ++i)
        {
            HAL::FramebufferDesc framebufferDesc{};
            framebufferDesc.RenderPass        = m_RenderPass.GetRaw();
            framebufferDesc.RenderTargetViews = { m_RTVs[i] };
            framebufferDesc.Width             = m_Scissor.Width();
            framebufferDesc.Height            = m_Scissor.Height();
            auto framebuffer                  = m_Framebuffers.Push(m_Device->CreateFramebuffer(framebufferDesc));

            auto& cmd = m_CommandBuffers.Push(m_Device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.GetRaw());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_VertexBuffer.GetRaw());
            cmd->BeginRenderPass(
                m_RenderPass.GetRaw(), framebuffer.GetRaw(), { HAL::ClearValueDesc{ FE::Colors::MediumAquamarine } });
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
