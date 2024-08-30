#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <OsGPU/OsmiumGPU.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/CoreEvents.h>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
    [[maybe_unused]] FE::Float32 RGB[3];
};

namespace HAL = FE::Osmium;

inline constexpr const char* ExampleName = "Ferrum3D - Triangle with MSAA";
inline constexpr FE::int32_t MSAASamples   = 8;

void RunExample()
{
    auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();
    auto eventBus = FE::MakeShared<FE::EventBus<FE::FrameEvents>>();

    FE::DynamicLibrary osmiumLib;
    osmiumLib.LoadFrom("OsGPU");
    auto attachEnvironment = osmiumLib.GetFunction<HAL::AttachEnvironmentProc>("AttachEnvironment");
    attachEnvironment(&FE::Env::GetEnvironment());
    auto createGraphicsAPIInstance = osmiumLib.GetFunction<HAL::CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

    auto instance =
        FE::Rc<HAL::IInstance>(createGraphicsAPIInstance(HAL::InstanceDesc{ ExampleName }, HAL::GraphicsAPI::Vulkan));
    instance->ReleaseStrongRef();
    auto adapter       = instance->GetAdapters().Front();
    auto device        = adapter->CreateDevice();
    auto graphicsQueue = device->GetCommandQueue(HAL::CommandQueueClass::Graphics);

    auto window   = device->CreateWindow(HAL::WindowDesc{ 800, 600, ExampleName });
    auto viewport = window->CreateViewport();
    auto scissor  = window->CreateScissor();

    HAL::SwapchainDesc swapChainDesc{};
    swapChainDesc.ImageCount         = 3;
    swapChainDesc.ImageWidth         = scissor.Width();
    swapChainDesc.ImageHeight        = scissor.Height();
    swapChainDesc.NativeWindowHandle = window->GetNativeHandle();
    swapChainDesc.Queue              = graphicsQueue.Get();
    auto swapChain                   = device->CreateSwapChain(swapChainDesc);

    auto colorImageDesc = HAL::ImageDesc::Img2D(
        HAL::ImageBindFlags::kColor, scissor.Width(), scissor.Height(), swapChain->GetDesc().Format, false, MSAASamples);
    auto colorImage = device->CreateImage(colorImageDesc);
    colorImage->AllocateMemory(HAL::MemoryType::kDeviceLocal);
    auto colorImageView = colorImage->CreateView(HAL::ImageAspectFlags::kColor);

    FE::Rc<HAL::IBuffer> vertexBuffer;
    {
        // clang-format off
        FE::List<Vertex> vertexData = {
            {{+0.4f, -0.5f, 0}, {1, 0, 0}},
            {{+0.5f, +0.5f, 0}, {0, 1, 0}},
            {{-0.5f, +0.4f, 0}, {0, 0, 1}}
        };
        // clang-format on
        vertexBuffer = device->CreateBuffer(HAL::BufferDesc{ vertexData.Size() * sizeof(Vertex), HAL::BindFlags::VertexBuffer });
        vertexBuffer->AllocateMemory(HAL::MemoryType::kHostVisible);
        vertexBuffer->UpdateData(vertexData.Data());
    }

    auto compiler = device->CreateShaderCompiler();
    HAL::ShaderCompilerArgs shaderArgs{};
    shaderArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    shaderArgs.EntryPoint = "main";

    shaderArgs.Stage      = HAL::ShaderStage::kPixel;
    shaderArgs.FullPath   = "../../Samples/Triangle/Shaders/PixelShader.hlsl";
    auto source           = FE::IO::File::ReadAllText(shaderArgs.FullPath);
    shaderArgs.SourceCode = source;
    auto psByteCode       = compiler->CompileShader(shaderArgs);

    shaderArgs.Stage      = HAL::ShaderStage::kVertex;
    shaderArgs.FullPath   = "../../Samples/Triangle/Shaders/VertexShader.hlsl";
    source                = FE::IO::File::ReadAllText(shaderArgs.FullPath);
    shaderArgs.SourceCode = source;
    auto vsByteCode       = compiler->CompileShader(shaderArgs);
    compiler.Reset();

    auto pixelShader  = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::kPixel, psByteCode));
    auto vertexShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::kVertex, vsByteCode));

    HAL::RenderPassDesc renderPassDesc{};

    HAL::AttachmentDesc colorAttachmentDesc{};
    colorAttachmentDesc.Format       = swapChain->GetDesc().Format;
    colorAttachmentDesc.InitialState = HAL::ResourceState::kUndefined;
    colorAttachmentDesc.FinalState   = HAL::ResourceState::kRenderTarget;
    colorAttachmentDesc.SampleCount  = MSAASamples;

    HAL::AttachmentDesc resolveAttachmentDesc{};
    resolveAttachmentDesc.Format       = swapChain->GetDesc().Format;
    resolveAttachmentDesc.InitialState = HAL::ResourceState::kUndefined;
    resolveAttachmentDesc.FinalState   = HAL::ResourceState::kPresent;
    renderPassDesc.Attachments         = { colorAttachmentDesc, resolveAttachmentDesc };

    HAL::SubpassDesc subpassDesc{};
    subpassDesc.RenderTargetAttachments = { HAL::SubpassAttachment(HAL::ResourceState::kRenderTarget, 0) };
    subpassDesc.MSAAResolveAttachments  = { HAL::SubpassAttachment(HAL::ResourceState::kRenderTarget, 1) };
    renderPassDesc.Subpasses            = { subpassDesc };
    HAL::SubpassDependency dependency{};
    renderPassDesc.SubpassDependencies = { dependency };
    auto renderPass                    = device->CreateRenderPass(renderPassDesc);

    HAL::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = HAL::InputLayoutBuilder{}
                                   .AddBuffer(HAL::InputStreamRate::kPerVertex)
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "COLOR")
                                   .Build()
                                   .Build();

    FE::List shaders{ pixelShader.Get(), vertexShader.Get() };

    pipelineDesc.RenderPass             = renderPass.Get();
    pipelineDesc.SubpassIndex           = 0;
    pipelineDesc.ColorBlend             = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
    pipelineDesc.Shaders                = shaders;
    pipelineDesc.Multisample            = HAL::MultisampleState(MSAASamples, 0.2f, true);
    pipelineDesc.Rasterization          = HAL::RasterizationState{};
    pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::kBack;
    pipelineDesc.Scissor                = scissor;
    pipelineDesc.Viewport               = viewport;

    auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

    FE::List<FE::Rc<HAL::IFence>> fences;
    for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
        fences.Push(device->CreateFence(HAL::FenceState::Signaled));

    auto RTVs = swapChain->GetRTVs();
    FE::List<FE::Rc<HAL::Framebuffer>> framebuffers;
    FE::List<FE::Rc<HAL::ICommandBuffer>> commandBuffers;
    for (size_t i = 0; i < swapChain->GetImageCount(); ++i)
    {
        FE::List framebufferRTVs{ colorImageView.Get(), RTVs[i] };

        HAL::FramebufferDesc framebufferDesc{};
        framebufferDesc.RenderPass        = renderPass.Get();
        framebufferDesc.RenderTargetViews = framebufferRTVs;
        framebufferDesc.Width             = scissor.Width();
        framebufferDesc.Height            = scissor.Height();
        auto framebuffer                  = framebuffers.Emplace(device->CreateFramebuffer(framebufferDesc));

        auto& cmd = commandBuffers.Emplace(device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
        cmd->Begin();
        cmd->BindGraphicsPipeline(pipeline.Get());
        cmd->SetViewport(viewport);
        cmd->SetScissor(scissor);
        cmd->BindVertexBuffer(0, vertexBuffer.Get(), 0);
        cmd->BeginRenderPass(renderPass.Get(),
                             framebuffer.Get(),
                             { HAL::ClearValueDesc::CreateColorValue(FE::Colors::MediumAquamarine),
                               HAL::ClearValueDesc::CreateColorValue(FE::Colors::MediumAquamarine) });
        cmd->Draw(3, 1, 0, 0);
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
        graphicsQueue->SubmitBuffers(
            { commandBuffers[imageIndex].Get() }, fences[frameIndex].Get(), HAL::SubmitFlags::FrameBeginEnd);
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
