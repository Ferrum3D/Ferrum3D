#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <OsGPU/OsmiumGPU.h>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
};

namespace HAL = FE::Osmium;

inline constexpr const char* ExampleName = "Ferrum3D - Uniforms";

void RunExample()
{
    auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();

    FE::DynamicLibrary osmiumLib("OsGPU");
    auto attachEnvironment = osmiumLib.GetFunction<RHI::AttachEnvironmentProc>("AttachEnvironment");
    attachEnvironment(&FE::Env::GetEnvironment());
    auto createGraphicsAPIInstance = osmiumLib.GetFunction<RHI::CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

    auto instance =
        FE::Rc<RHI::IInstance>(createGraphicsAPIInstance(RHI::InstanceDesc{ ExampleName }, RHI::GraphicsAPI::Vulkan));
    instance->ReleaseStrongRef();
    auto adapter       = instance->GetAdapters().front();
    auto device        = adapter->CreateDevice();
    auto transferQueue = device->GetCommandQueue(RHI::CommandQueueClass::Transfer);
    auto graphicsQueue = device->GetCommandQueue(RHI::CommandQueueClass::Graphics);

    auto window   = device->CreateWindow(RHI::WindowDesc{ 800, 600, ExampleName });
    auto viewport = window->CreateViewport();
    auto scissor  = window->CreateScissor();

    RHI::SwapchainDesc swapChainDesc{};
    swapChainDesc.ImageCount         = 3;
    swapChainDesc.ImageWidth         = scissor.Width();
    swapChainDesc.ImageHeight        = scissor.Height();
    swapChainDesc.NativeWindowHandle = window->GetNativeHandle();
    swapChainDesc.Queue              = graphicsQueue.Get();
    auto swapChain                   = device->CreateSwapChain(swapChainDesc);

    FE::Rc<RHI::IBuffer> indexBufferStaging, vertexBufferStaging;
    FE::Rc<RHI::IBuffer> psConstantBuffer, vsConstantBuffer;
    FE::Rc<RHI::IBuffer> indexBuffer, vertexBuffer;
    uint64_t vertexSize, indexSize;
    {
        // clang-format off
            FE::List<Vertex> vertexData = {
                { {-0.5f, -0.5f, 0.0f} },
                { {+0.5f, +0.5f, 0.0f} },
                { {+0.5f, -0.5f, 0.0f} },
                { {-0.5f, +0.5f, 0.0f} }
            };
        // clang-format on
        vertexSize          = vertexData.Size() * sizeof(Vertex);
        vertexBufferStaging = device->CreateBuffer(RHI::BindFlags::None, vertexSize);
        vertexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
        vertexBufferStaging->UpdateData(vertexData.Data());

        vertexBuffer = device->CreateBuffer(RHI::BindFlags::VertexBuffer, vertexSize);
        vertexBuffer->AllocateMemory(RHI::MemoryType::kDeviceLocal);
    }
    {
        FE::List<uint32_t> indexData = { 0, 2, 3, 3, 2, 1 };
        indexSize                        = indexData.Size() * sizeof(uint32_t);
        indexBufferStaging               = device->CreateBuffer(RHI::BindFlags::None, indexSize);
        indexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
        indexBufferStaging->UpdateData(indexData.Data());

        indexBuffer = device->CreateBuffer(RHI::BindFlags::IndexBuffer, indexSize);
        indexBuffer->AllocateMemory(RHI::MemoryType::kDeviceLocal);
    }

    {
        auto constantData = FE::Colors::Gold;
        psConstantBuffer       = device->CreateBuffer(RHI::BindFlags::ConstantBuffer, sizeof(FE::Vector4F));
        psConstantBuffer->AllocateMemory(RHI::MemoryType::kHostVisible);
        psConstantBuffer->UpdateData(constantData.Data());
    }
    {
        FE::Vector3F constantData = { 0.3f, -0.4f, 0.0f };
        vsConstantBuffer          = device->CreateBuffer(RHI::BindFlags::ConstantBuffer, sizeof(FE::Vector3F));
        vsConstantBuffer->AllocateMemory(RHI::MemoryType::kHostVisible);
        vsConstantBuffer->UpdateData(constantData.Data());
    }

    {
        auto transferComplete = device->CreateFence(RHI::FenceState::Reset);
        auto copyCmdBuffer    = device->CreateCommandBuffer(RHI::CommandQueueClass::Transfer);
        copyCmdBuffer->Begin();
        copyCmdBuffer->CopyBuffers(vertexBufferStaging.Get(), vertexBuffer.Get(), RHI::BufferCopyRegion(vertexSize));
        copyCmdBuffer->CopyBuffers(indexBufferStaging.Get(), indexBuffer.Get(), RHI::BufferCopyRegion(indexSize));
        copyCmdBuffer->End();
        transferQueue->SubmitBuffers({ copyCmdBuffer.Get() }, { transferComplete }, RHI::SubmitFlags::None);
        transferComplete->WaitOnCPU();
    }

    auto compiler = device->CreateShaderCompiler();
    RHI::ShaderCompilerArgs psArgs{};
    psArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
    psArgs.Stage      = RHI::ShaderStage::kPixel;
    psArgs.EntryPoint = "main";
    psArgs.FullPath   = "../../Samples/Uniforms/Shaders/PixelShader.hlsl";
    auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
    psArgs.SourceCode = psSource;
    auto psByteCode   = compiler->CompileShader(psArgs);

    auto pixelShader = device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kPixel, psByteCode));

    RHI::ShaderCompilerArgs vsArgs{};
    vsArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
    vsArgs.Stage      = RHI::ShaderStage::kVertex;
    vsArgs.EntryPoint = "main";
    vsArgs.FullPath   = "../../Samples/Uniforms/Shaders/VertexShader.hlsl";
    auto vsSource     = FE::IO::File::ReadAllText(vsArgs.FullPath);
    vsArgs.SourceCode = vsSource;
    auto vsByteCode   = compiler->CompileShader(vsArgs);

    auto vertexShader = device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kVertex, vsByteCode));

    RHI::RenderPassDesc renderPassDesc{};

    RHI::AttachmentDesc attachmentDesc{};
    attachmentDesc.Format       = swapChain->GetDesc().Format;
    attachmentDesc.StoreOp      = RHI::AttachmentStoreOp::Store;
    attachmentDesc.LoadOp       = RHI::AttachmentLoadOp::Clear;
    attachmentDesc.InitialState = RHI::ResourceState::kUndefined;
    attachmentDesc.FinalState   = RHI::ResourceState::kPresent;

    renderPassDesc.Attachments = { attachmentDesc };

    RHI::SubpassDesc subpassDesc{};
    subpassDesc.RenderTargetAttachments = { RHI::SubpassAttachment(RHI::ResourceState::kRenderTarget, 0) };
    renderPassDesc.Subpasses            = { subpassDesc };

    RHI::SubpassDependency dependency{};
    renderPassDesc.SubpassDependencies = { dependency };

    auto renderPass = device->CreateRenderPass(renderPassDesc);

    RHI::DescriptorHeapDesc descriptorHeapDesc{};
    descriptorHeapDesc.MaxTables = 2;
    descriptorHeapDesc.Sizes     = { RHI::DescriptorSize(1, RHI::ShaderResourceType::ConstantBuffer),
                                     RHI::DescriptorSize(1, RHI::ShaderResourceType::ConstantBuffer) };
    auto descriptorHeap          = device->CreateDescriptorHeap(descriptorHeapDesc);

    RHI::DescriptorDesc psDescriptorDesc(RHI::ShaderResourceType::ConstantBuffer, RHI::ShaderStageFlags::kPixel, 1);
    RHI::DescriptorDesc vsDescriptorDesc(RHI::ShaderResourceType::ConstantBuffer, RHI::ShaderStageFlags::kVertex, 1);
    auto descriptorTable = descriptorHeap->AllocateDescriptorTable({ psDescriptorDesc, vsDescriptorDesc });

    RHI::DescriptorWriteBuffer descriptorWrite{ psConstantBuffer.Get() };
    descriptorTable->Update(descriptorWrite);
    descriptorWrite.Binding = 1;
    descriptorWrite.Buffer  = vsConstantBuffer.Get();
    descriptorTable->Update(descriptorWrite);

    RHI::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = RHI::InputLayoutBuilder(RHI::PrimitiveTopology::kTriangleList)
                                   .AddBuffer(RHI::InputStreamRate::kPerVertex)
                                   .AddAttribute(RHI::Format::R32G32B32_SFloat, "POSITION")
                                   .Build()
                                   .Build();

    pipelineDesc.RenderPass       = renderPass;
    pipelineDesc.SubpassIndex     = 0;
    pipelineDesc.ColorBlend       = RHI::ColorBlendState({ RHI::TargetColorBlending{} });
    pipelineDesc.Shaders          = { pixelShader, vertexShader };
    pipelineDesc.DescriptorTables = { descriptorTable };
    pipelineDesc.Viewport         = viewport;
    pipelineDesc.Scissor          = scissor;
    pipelineDesc.Rasterization    = RHI::RasterizationState{};

    pipelineDesc.Rasterization.CullMode = RHI::CullingModeFlags::kBack;

    auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

    FE::List<FE::Rc<RHI::IFence>> fences;
    for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
    {
        fences.Push(device->CreateFence(RHI::FenceState::Signaled));
    }

    auto RTVs = swapChain->GetRTVs();
    FE::List<FE::Rc<RHI::Framebuffer>> framebuffers;
    FE::List<FE::Rc<RHI::ICommandBuffer>> commandBuffers;
    for (size_t i = 0; i < swapChain->GetImageCount(); ++i)
    {
        RHI::FramebufferDesc framebufferDesc{};
        framebufferDesc.RenderPass        = renderPass.Get();
        framebufferDesc.RenderTargetViews = { RTVs[i] };
        framebufferDesc.Width             = scissor.Width();
        framebufferDesc.Height            = scissor.Height();
        auto framebuffer                  = framebuffers.Emplace(device->CreateFramebuffer(framebufferDesc));

        auto& cmd = commandBuffers.Emplace(device->CreateCommandBuffer(RHI::CommandQueueClass::Graphics));
        cmd->Begin();
        cmd->BindGraphicsPipeline(pipeline.Get());
        cmd->BindDescriptorTables({ descriptorTable.Get() }, pipeline.Get());
        cmd->SetViewport(viewport);
        cmd->SetScissor(scissor);
        cmd->BindVertexBuffer(0, vertexBuffer.Get());
        cmd->BindIndexBuffer(indexBuffer.Get());
        cmd->BeginRenderPass(renderPass.Get(), framebuffer.Get(), { RHI::ClearValueDesc{ FE::Colors::MediumAquamarine } });
        cmd->DrawIndexed(6, 1, 0, 0, 0);
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
            { commandBuffers[imageIndex].Get() }, fences[frameIndex], RHI::SubmitFlags::FrameBeginEnd);
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
