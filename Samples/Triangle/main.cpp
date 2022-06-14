#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <GPU/Instance/IInstance.h>
#include <GPU/Pipeline/InputLayoutBuilder.h>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
    [[maybe_unused]] FE::Float32 RGB[3];
};

namespace HAL = FE::GPU;

void RunExample()
{
    auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();

    FE::DynamicLibrary osmiumLib("OsmiumGPU");
    auto attachEnvironment = osmiumLib.GetFunction<HAL::AttachEnvironmentProc>("AttachEnvironment");
    attachEnvironment(&FE::Env::GetEnvironment());
    auto createGraphicsAPIInstance = osmiumLib.GetFunction<HAL::CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

    auto instance = FE::Shared<HAL::IInstance>(createGraphicsAPIInstance(HAL::InstanceDesc{}, HAL::GraphicsAPI::Vulkan));
    instance->ReleaseStrongRef();
    auto adapter       = instance->GetAdapters().front();
    auto device        = adapter->CreateDevice();
    auto graphicsQueue = device->GetCommandQueue(HAL::CommandQueueClass::Graphics);

    auto window   = device->CreateWindow(HAL::WindowDesc{ 800, 600, "Test project" });
    auto viewport = window->CreateViewport();
    auto scissor  = window->CreateScissor();

    HAL::SwapChainDesc swapChainDesc{};
    swapChainDesc.ImageCount         = 3;
    swapChainDesc.ImageWidth         = scissor.Width();
    swapChainDesc.ImageHeight        = scissor.Height();
    swapChainDesc.NativeWindowHandle = window->GetNativeHandle();
    swapChainDesc.Queue              = graphicsQueue.GetRaw();
    auto swapChain                   = device->CreateSwapChain(swapChainDesc);

    FE::Shared<HAL::IBuffer> vertexBuffer;
    {
        // clang-format off
        FE::Vector<Vertex> vertexData = {
            {{+0.0, -0.5, 0}, {1, 0, 0}},
            {{+0.5, +0.5, 0}, {0, 1, 0}},
            {{-0.5, +0.5, 0}, {0, 0, 1}}
        };
        // clang-format on
        vertexBuffer = device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexData.size() * sizeof(Vertex));
        vertexBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
        vertexBuffer->UpdateData(vertexData.data());
    }

    auto compiler = device->CreateShaderCompiler();
    HAL::ShaderCompilerArgs shaderArgs{};
    shaderArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    shaderArgs.EntryPoint = "main";

    shaderArgs.Stage      = HAL::ShaderStage::Pixel;
    shaderArgs.FullPath   = "../Assets/Samples/Triangle/Shaders/PixelShader.hlsl";
    auto source           = FE::IO::File::ReadAllText(shaderArgs.FullPath);
    shaderArgs.SourceCode = source;
    auto psByteCode       = compiler->CompileShader(shaderArgs);

    shaderArgs.Stage      = HAL::ShaderStage::Vertex;
    shaderArgs.FullPath   = "../Assets/Samples/Triangle/Shaders/VertexShader.hlsl";
    source                = FE::IO::File::ReadAllText(shaderArgs.FullPath);
    shaderArgs.SourceCode = source;
    auto vsByteCode       = compiler->CompileShader(shaderArgs);
    compiler.Reset();

    auto pixelShader  = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));
    auto vertexShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Vertex, vsByteCode));

    HAL::RenderPassDesc renderPassDesc{};

    HAL::AttachmentDesc attachmentDesc{};
    attachmentDesc.Format       = swapChain->GetDesc().Format;
    attachmentDesc.InitialState = HAL::ResourceState::Undefined;
    attachmentDesc.FinalState   = HAL::ResourceState::Present;
    renderPassDesc.Attachments  = { attachmentDesc };

    HAL::SubpassDesc subpassDesc{};
    subpassDesc.RenderTargetAttachments = { HAL::SubpassAttachment(HAL::ResourceState::RenderTarget, 0) };
    renderPassDesc.Subpasses            = { subpassDesc };
    HAL::SubpassDependency dependency{};
    renderPassDesc.SubpassDependencies = { dependency };
    auto renderPass                    = device->CreateRenderPass(renderPassDesc);

    HAL::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = HAL::InputLayoutBuilder{}
                                   .AddBuffer(HAL::InputStreamRate::PerVertex)
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "COLOR")
                                   .Build()
                                   .Build();

    pipelineDesc.RenderPass             = renderPass;
    pipelineDesc.SubpassIndex           = 0;
    pipelineDesc.ColorBlend             = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
    pipelineDesc.Shaders                = { pixelShader, vertexShader };
    pipelineDesc.Rasterization          = HAL::RasterizationState{};
    pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

    auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

    FE::Vector<FE::Shared<HAL::IFence>> fences;
    for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
        fences.push_back(device->CreateFence(HAL::FenceState::Signaled));

    auto RTVs = swapChain->GetRTVs();
    FE::Vector<FE::Shared<HAL::IFramebuffer>> framebuffers;
    FE::Vector<FE::Shared<HAL::ICommandBuffer>> commandBuffers;
    for (size_t i = 0; i < swapChain->GetImageCount(); ++i)
    {
        HAL::FramebufferDesc framebufferDesc{};
        framebufferDesc.RenderPass        = renderPass.GetRaw();
        framebufferDesc.RenderTargetViews = { RTVs[i] };
        framebufferDesc.Width             = scissor.Width();
        framebufferDesc.Height            = scissor.Height();
        auto framebuffer                  = framebuffers.emplace_back(device->CreateFramebuffer(framebufferDesc));

        auto& cmd = commandBuffers.emplace_back(device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
        cmd->Begin();
        cmd->BindGraphicsPipeline(pipeline.GetRaw());
        cmd->SetViewport(viewport);
        cmd->SetScissor(scissor);
        cmd->BindVertexBuffer(0, vertexBuffer.GetRaw());
        cmd->BeginRenderPass(renderPass.GetRaw(), framebuffer.GetRaw(), HAL::ClearValueDesc{ FE::Colors::MediumAquamarine });
        cmd->Draw(6, 1, 0, 0);
        cmd->EndRenderPass();
        cmd->End();
    }

    while (!window->CloseRequested())
    {
        auto frameIndex = swapChain->GetCurrentFrameIndex();

        fences[frameIndex]->WaitOnCPU();
        window->PollEvents();
        auto imageIndex = swapChain->GetCurrentImageIndex();
        fences[swapChain->GetCurrentFrameIndex()]->Reset();
        graphicsQueue->SubmitBuffers({ commandBuffers[imageIndex] }, fences[frameIndex], HAL::SubmitFlags::FrameBeginEnd);
        swapChain->Present();
    }

    device->WaitIdle();
}

int main()
{
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc());
    RunExample();
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
    FE::Env::DetachEnvironment();
}
