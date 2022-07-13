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
    auto attachEnvironment = osmiumLib.GetFunction<HAL::AttachEnvironmentProc>("AttachEnvironment");
    attachEnvironment(&FE::Env::GetEnvironment());
    auto createGraphicsAPIInstance = osmiumLib.GetFunction<HAL::CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

    auto instance =
        FE::Shared<HAL::IInstance>(createGraphicsAPIInstance(HAL::InstanceDesc{ ExampleName }, HAL::GraphicsAPI::Vulkan));
    instance->ReleaseStrongRef();
    auto adapter       = instance->GetAdapters().front();
    auto device        = adapter->CreateDevice();
    auto transferQueue = device->GetCommandQueue(HAL::CommandQueueClass::Transfer);
    auto graphicsQueue = device->GetCommandQueue(HAL::CommandQueueClass::Graphics);

    auto window   = device->CreateWindow(HAL::WindowDesc{ 800, 600, ExampleName });
    auto viewport = window->CreateViewport();
    auto scissor  = window->CreateScissor();

    HAL::SwapChainDesc swapChainDesc{};
    swapChainDesc.ImageCount         = 3;
    swapChainDesc.ImageWidth         = scissor.Width();
    swapChainDesc.ImageHeight        = scissor.Height();
    swapChainDesc.NativeWindowHandle = window->GetNativeHandle();
    swapChainDesc.Queue              = graphicsQueue.GetRaw();
    auto swapChain                   = device->CreateSwapChain(swapChainDesc);

    FE::Shared<HAL::IBuffer> indexBufferStaging, vertexBufferStaging;
    FE::Shared<HAL::IBuffer> psConstantBuffer, vsConstantBuffer;
    FE::Shared<HAL::IBuffer> indexBuffer, vertexBuffer;
    FE::UInt64 vertexSize, indexSize;
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
        vertexBufferStaging = device->CreateBuffer(HAL::BindFlags::None, vertexSize);
        vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        vertexBufferStaging->UpdateData(vertexData.Data());

        vertexBuffer = device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexSize);
        vertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }
    {
        FE::List<FE::UInt32> indexData = { 0, 2, 3, 3, 2, 1 };
        indexSize                        = indexData.Size() * sizeof(FE::UInt32);
        indexBufferStaging               = device->CreateBuffer(HAL::BindFlags::None, indexSize);
        indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        indexBufferStaging->UpdateData(indexData.Data());

        indexBuffer = device->CreateBuffer(HAL::BindFlags::IndexBuffer, indexSize);
        indexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }

    {
        auto constantData = FE::Colors::Gold;
        psConstantBuffer       = device->CreateBuffer(HAL::BindFlags::ConstantBuffer, sizeof(FE::Vector4F));
        psConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
        psConstantBuffer->UpdateData(constantData.Data());
    }
    {
        FE::Vector3F constantData = { 0.3f, -0.4f, 0.0f };
        vsConstantBuffer          = device->CreateBuffer(HAL::BindFlags::ConstantBuffer, sizeof(FE::Vector3F));
        vsConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
        vsConstantBuffer->UpdateData(constantData.Data());
    }

    {
        auto transferComplete = device->CreateFence(HAL::FenceState::Reset);
        auto copyCmdBuffer    = device->CreateCommandBuffer(HAL::CommandQueueClass::Transfer);
        copyCmdBuffer->Begin();
        copyCmdBuffer->CopyBuffers(vertexBufferStaging.GetRaw(), vertexBuffer.GetRaw(), HAL::BufferCopyRegion(vertexSize));
        copyCmdBuffer->CopyBuffers(indexBufferStaging.GetRaw(), indexBuffer.GetRaw(), HAL::BufferCopyRegion(indexSize));
        copyCmdBuffer->End();
        transferQueue->SubmitBuffers({ copyCmdBuffer.GetRaw() }, { transferComplete }, HAL::SubmitFlags::None);
        transferComplete->WaitOnCPU();
    }

    auto compiler = device->CreateShaderCompiler();
    HAL::ShaderCompilerArgs psArgs{};
    psArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    psArgs.Stage      = HAL::ShaderStage::Pixel;
    psArgs.EntryPoint = "main";
    psArgs.FullPath   = "../../Samples/Uniforms/Shaders/PixelShader.hlsl";
    auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
    psArgs.SourceCode = psSource;
    auto psByteCode   = compiler->CompileShader(psArgs);

    auto pixelShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));

    HAL::ShaderCompilerArgs vsArgs{};
    vsArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    vsArgs.Stage      = HAL::ShaderStage::Vertex;
    vsArgs.EntryPoint = "main";
    vsArgs.FullPath   = "../../Samples/Uniforms/Shaders/VertexShader.hlsl";
    auto vsSource     = FE::IO::File::ReadAllText(vsArgs.FullPath);
    vsArgs.SourceCode = vsSource;
    auto vsByteCode   = compiler->CompileShader(vsArgs);

    auto vertexShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Vertex, vsByteCode));

    HAL::RenderPassDesc renderPassDesc{};

    HAL::AttachmentDesc attachmentDesc{};
    attachmentDesc.Format       = swapChain->GetDesc().Format;
    attachmentDesc.StoreOp      = HAL::AttachmentStoreOp::Store;
    attachmentDesc.LoadOp       = HAL::AttachmentLoadOp::Clear;
    attachmentDesc.InitialState = HAL::ResourceState::Undefined;
    attachmentDesc.FinalState   = HAL::ResourceState::Present;

    renderPassDesc.Attachments = { attachmentDesc };

    HAL::SubpassDesc subpassDesc{};
    subpassDesc.RenderTargetAttachments = { HAL::SubpassAttachment(HAL::ResourceState::RenderTarget, 0) };
    renderPassDesc.Subpasses            = { subpassDesc };

    HAL::SubpassDependency dependency{};
    renderPassDesc.SubpassDependencies = { dependency };

    auto renderPass = device->CreateRenderPass(renderPassDesc);

    HAL::DescriptorHeapDesc descriptorHeapDesc{};
    descriptorHeapDesc.MaxTables = 2;
    descriptorHeapDesc.Sizes     = { HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer),
                                     HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer) };
    auto descriptorHeap          = device->CreateDescriptorHeap(descriptorHeapDesc);

    HAL::DescriptorDesc psDescriptorDesc(HAL::ShaderResourceType::ConstantBuffer, HAL::ShaderStageFlags::Pixel, 1);
    HAL::DescriptorDesc vsDescriptorDesc(HAL::ShaderResourceType::ConstantBuffer, HAL::ShaderStageFlags::Vertex, 1);
    auto descriptorTable = descriptorHeap->AllocateDescriptorTable({ psDescriptorDesc, vsDescriptorDesc });

    HAL::DescriptorWriteBuffer descriptorWrite{ psConstantBuffer.GetRaw() };
    descriptorTable->Update(descriptorWrite);
    descriptorWrite.Binding = 1;
    descriptorWrite.Buffer  = vsConstantBuffer.GetRaw();
    descriptorTable->Update(descriptorWrite);

    HAL::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
                                   .AddBuffer(HAL::InputStreamRate::PerVertex)
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                   .Build()
                                   .Build();

    pipelineDesc.RenderPass       = renderPass;
    pipelineDesc.SubpassIndex     = 0;
    pipelineDesc.ColorBlend       = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
    pipelineDesc.Shaders          = { pixelShader, vertexShader };
    pipelineDesc.DescriptorTables = { descriptorTable };
    pipelineDesc.Viewport         = viewport;
    pipelineDesc.Scissor          = scissor;
    pipelineDesc.Rasterization    = HAL::RasterizationState{};

    pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

    auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

    FE::List<FE::Shared<HAL::IFence>> fences;
    for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
    {
        fences.Push(device->CreateFence(HAL::FenceState::Signaled));
    }

    auto RTVs = swapChain->GetRTVs();
    FE::List<FE::Shared<HAL::IFramebuffer>> framebuffers;
    FE::List<FE::Shared<HAL::ICommandBuffer>> commandBuffers;
    for (size_t i = 0; i < swapChain->GetImageCount(); ++i)
    {
        HAL::FramebufferDesc framebufferDesc{};
        framebufferDesc.RenderPass        = renderPass.GetRaw();
        framebufferDesc.RenderTargetViews = { RTVs[i] };
        framebufferDesc.Width             = scissor.Width();
        framebufferDesc.Height            = scissor.Height();
        auto framebuffer                  = framebuffers.Emplace(device->CreateFramebuffer(framebufferDesc));

        auto& cmd = commandBuffers.Emplace(device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
        cmd->Begin();
        cmd->BindGraphicsPipeline(pipeline.GetRaw());
        cmd->BindDescriptorTables({ descriptorTable.GetRaw() }, pipeline.GetRaw());
        cmd->SetViewport(viewport);
        cmd->SetScissor(scissor);
        cmd->BindVertexBuffer(0, vertexBuffer.GetRaw());
        cmd->BindIndexBuffer(indexBuffer.GetRaw());
        cmd->BeginRenderPass(renderPass.GetRaw(), framebuffer.GetRaw(), { HAL::ClearValueDesc{ FE::Colors::MediumAquamarine } });
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
            { commandBuffers[imageIndex].GetRaw() }, fences[frameIndex], HAL::SubmitFlags::FrameBeginEnd);
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
