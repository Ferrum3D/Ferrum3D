#include <FeCore/Assets/Asset.h>
#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Images/ImageAssetStorage.h>
#include <OsAssets/Meshes/MeshAssetLoader.h>
#include <OsAssets/Meshes/MeshAssetStorage.h>
#include <OsGPU/OsmiumGPU.h>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
    [[maybe_unused]] FE::Float32 UV[2];
};

namespace HAL = FE::Osmium;
using FE::static_pointer_cast;

inline constexpr const char* ExampleName = "Ferrum3D - Models";

void RunExample()
{
    auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();

    auto assetManager  = FE::MakeShared<FE::Assets::AssetManager>();
    auto assetProvider = FE::MakeShared<FE::Assets::AssetProviderDev>();
    auto assetRegistry = FE::MakeShared<FE::Assets::AssetRegistry>();
    assetRegistry->LoadAssetsFromFile("../Assets/Samples/Models/FerrumAssetIndex");
    assetProvider->AttachRegistry(assetRegistry);
    assetManager->RegisterAssetLoader(static_pointer_cast<FE::Assets::IAssetLoader>(FE::MakeShared<HAL::ImageAssetLoader>()));
    assetManager->RegisterAssetLoader(static_pointer_cast<FE::Assets::IAssetLoader>(FE::MakeShared<HAL::MeshAssetLoader>()));
    assetManager->AttachAssetProvider(static_pointer_cast<FE::Assets::IAssetProvider>(assetProvider));

    FE::DynamicLibrary osmiumLib("OsmiumGPU");
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

    auto meshAsset = FE::Assets::Asset<HAL::MeshAssetStorage>(FE::Assets::AssetID("884FEDDD-141D-49A0-92B2-38B519403D0A"));
    meshAsset.LoadSync();
    FE::Shared<HAL::IBuffer> indexBufferStaging, vertexBufferStaging;
    FE::Shared<HAL::IBuffer> vsConstantBuffer;
    FE::Shared<HAL::IBuffer> indexBuffer, vertexBuffer;
    FE::UInt64 vertexSize, indexSize;
    {
        vertexSize          = meshAsset->VertexSize();
        vertexBufferStaging = device->CreateBuffer(HAL::BindFlags::None, vertexSize);
        vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        vertexBufferStaging->UpdateData(meshAsset->VertexData());

        vertexBuffer = device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexSize);
        vertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }
    {
        indexSize          = meshAsset->IndexSize();
        indexBufferStaging = device->CreateBuffer(HAL::BindFlags::None, indexSize);
        indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        indexBufferStaging->UpdateData(meshAsset->IndexData());

        indexBuffer = device->CreateBuffer(HAL::BindFlags::IndexBuffer, indexSize);
        indexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }

    FE::Shared<HAL::IBuffer> textureStaging;
    FE::Shared<HAL::IImage> textureImage;
    {
        auto imageAsset = FE::Assets::Asset<HAL::ImageAssetStorage>(FE::Assets::AssetID("94FC6391-4656-4BE7-844D-8D87680A00F1"));
        imageAsset.LoadSync();

        textureStaging = device->CreateBuffer(HAL::BindFlags::None, imageAsset->Size());
        textureStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        textureStaging->UpdateData(imageAsset->Data());

        auto imageDesc = HAL::ImageDesc::Img2D(
            HAL::ImageBindFlags::TransferWrite | HAL::ImageBindFlags::ShaderRead, imageAsset->Width(), imageAsset->Height(),
            HAL::Format::R8G8B8A8_SRGB);
        textureImage = device->CreateImage(imageDesc);
        textureImage->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }

    {
        auto aspectRatio =
            static_cast<float>(swapChain->GetDesc().ImageWidth) / static_cast<float>(swapChain->GetDesc().ImageHeight);
        auto constantData = FE::Matrix4x4F::GetIdentity();
        constantData *= FE::Matrix4x4F::CreateProjection(FE::Constants::PI * 0.5, aspectRatio, 0.1f, 10.0f);
        constantData *= FE::Matrix4x4F::CreateRotationY(FE::Constants::PI);
        constantData *= FE::Matrix4x4F::CreateRotationX(-0.5f);
        constantData *= FE::Matrix4x4F::CreateTranslation(FE::Vector3F(0.0f, 0.8f, -1.5f) * 2);
        constantData *= FE::Matrix4x4F::CreateRotationY(FE::Constants::PI * -1.3f);
        vsConstantBuffer = device->CreateBuffer(HAL::BindFlags::ConstantBuffer, sizeof(constantData));
        vsConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
        vsConstantBuffer->UpdateData(constantData.RowMajorData());
    }

    {
        auto transferComplete = device->CreateFence(HAL::FenceState::Reset);
        auto copyCmdBuffer    = device->CreateCommandBuffer(HAL::CommandQueueClass::Transfer);
        copyCmdBuffer->Begin();
        copyCmdBuffer->CopyBuffers(vertexBufferStaging.GetRaw(), vertexBuffer.GetRaw(), HAL::BufferCopyRegion(vertexSize));
        copyCmdBuffer->CopyBuffers(indexBufferStaging.GetRaw(), indexBuffer.GetRaw(), HAL::BufferCopyRegion(indexSize));

        HAL::ResourceTransitionBarrierDesc barrier{};
        barrier.Image                        = textureImage.GetRaw();
        barrier.SubresourceRange.AspectFlags = HAL::ImageAspectFlags::RenderTarget;
        barrier.StateBefore                  = HAL::ResourceState::Undefined;
        barrier.StateAfter                   = HAL::ResourceState::CopyDest;
        copyCmdBuffer->ResourceTransitionBarriers({ barrier });

        auto size = textureImage->GetDesc().ImageSize;
        copyCmdBuffer->CopyBufferToImage(textureStaging.GetRaw(), textureImage.GetRaw(), HAL::BufferImageCopyRegion(size));

        barrier.StateBefore = HAL::ResourceState::CopyDest;
        barrier.StateAfter  = HAL::ResourceState::ShaderResource;
        copyCmdBuffer->ResourceTransitionBarriers({ barrier });
        copyCmdBuffer->End();
        transferQueue->SubmitBuffers({ copyCmdBuffer.GetRaw() }, { transferComplete }, HAL::SubmitFlags::None);
        transferComplete->WaitOnCPU();
    }

    auto textureView    = textureImage->CreateView(HAL::ImageAspectFlags::RenderTarget);
    auto textureSampler = device->CreateSampler(HAL::SamplerDesc{});

    auto compiler = device->CreateShaderCompiler();
    HAL::ShaderCompilerArgs psArgs{};
    psArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    psArgs.Stage      = HAL::ShaderStage::Pixel;
    psArgs.EntryPoint = "main";
    psArgs.FullPath   = "../Assets/Samples/Models/Shaders/PixelShader.hlsl";
    auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
    psArgs.SourceCode = psSource;
    auto psByteCode   = compiler->CompileShader(psArgs);

    auto pixelShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));

    HAL::ShaderCompilerArgs vsArgs{};
    vsArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    vsArgs.Stage      = HAL::ShaderStage::Vertex;
    vsArgs.EntryPoint = "main";
    vsArgs.FullPath   = "../Assets/Samples/Models/Shaders/VertexShader.hlsl";
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

    HAL::AttachmentDesc depthAttachmentDesc{};
    depthAttachmentDesc.Format       = swapChain->GetDSV()->GetDesc().Format;
    depthAttachmentDesc.StoreOp      = HAL::AttachmentStoreOp::Store;
    depthAttachmentDesc.LoadOp       = HAL::AttachmentLoadOp::Clear;
    depthAttachmentDesc.InitialState = HAL::ResourceState::Undefined;
    depthAttachmentDesc.FinalState   = HAL::ResourceState::DepthWrite;

    renderPassDesc.Attachments = { attachmentDesc, depthAttachmentDesc };

    HAL::SubpassDesc subpassDesc{};
    subpassDesc.RenderTargetAttachments = { HAL::SubpassAttachment(HAL::ResourceState::RenderTarget, 0) };
    subpassDesc.DepthStencilAttachment  = HAL::SubpassAttachment(HAL::ResourceState::DepthWrite, 1);
    renderPassDesc.Subpasses            = { subpassDesc };

    HAL::SubpassDependency dependency{};
    renderPassDesc.SubpassDependencies = { dependency };

    auto renderPass = device->CreateRenderPass(renderPassDesc);

    HAL::DescriptorHeapDesc descriptorHeapDesc{};
    descriptorHeapDesc.MaxTables = 1;
    descriptorHeapDesc.Sizes     = { HAL::DescriptorSize(1, HAL::ShaderResourceType::Sampler),
                                     HAL::DescriptorSize(1, HAL::ShaderResourceType::TextureSRV),
                                     HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer) };
    auto descriptorHeap          = device->CreateDescriptorHeap(descriptorHeapDesc);

    HAL::DescriptorDesc psSamplerDescriptorDesc(HAL::ShaderResourceType::Sampler, HAL::ShaderStageFlags::Pixel, 1);
    HAL::DescriptorDesc psTextureDescriptorDesc(HAL::ShaderResourceType::TextureSRV, HAL::ShaderStageFlags::Pixel, 1);
    HAL::DescriptorDesc vsDescriptorDesc(HAL::ShaderResourceType::ConstantBuffer, HAL::ShaderStageFlags::Vertex, 1);
    auto descriptorTable =
        descriptorHeap->AllocateDescriptorTable({ psSamplerDescriptorDesc, psTextureDescriptorDesc, vsDescriptorDesc });

    HAL::DescriptorWriteSampler descriptorWriteSampler{ textureSampler.GetRaw() };
    descriptorTable->Update(descriptorWriteSampler);
    HAL::DescriptorWriteImage descriptorWriteImage{ textureView.GetRaw() };
    descriptorWriteImage.Binding = 1;
    descriptorTable->Update(descriptorWriteImage);
    HAL::DescriptorWriteBuffer descriptorWrite{ vsConstantBuffer.GetRaw() };
    descriptorWrite.Binding = 2;
    descriptorWrite.Buffer  = vsConstantBuffer.GetRaw();
    descriptorTable->Update(descriptorWrite);

    HAL::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
                                   .AddBuffer(HAL::InputStreamRate::PerVertex)
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                   .AddAttribute(HAL::Format::R32G32_SFloat, "TEXCOORD")
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

    pipelineDesc.DepthStencil.DepthWriteEnabled = true;
    pipelineDesc.DepthStencil.DepthTestEnabled  = true;
    pipelineDesc.DepthStencil.DepthCompareOp    = HAL::CompareOp::Less;

    pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

    auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

    FE::Vector<FE::Shared<HAL::IFence>> fences;
    for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
    {
        fences.push_back(device->CreateFence(HAL::FenceState::Signaled));
    }

    auto RTVs = swapChain->GetRTVs();
    FE::Vector<FE::Shared<HAL::IFramebuffer>> framebuffers;
    FE::Vector<FE::Shared<HAL::ICommandBuffer>> commandBuffers;
    for (size_t i = 0; i < swapChain->GetImageCount(); ++i)
    {
        HAL::FramebufferDesc framebufferDesc{};
        framebufferDesc.RenderPass        = renderPass.GetRaw();
        framebufferDesc.RenderTargetViews = { RTVs[i], swapChain->GetDSV() };
        framebufferDesc.Width             = scissor.Width();
        framebufferDesc.Height            = scissor.Height();
        auto framebuffer                  = framebuffers.emplace_back(device->CreateFramebuffer(framebufferDesc));

        auto& cmd = commandBuffers.emplace_back(device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
        cmd->Begin();
        cmd->BindGraphicsPipeline(pipeline.GetRaw());
        cmd->BindDescriptorTables({ descriptorTable.GetRaw() }, pipeline.GetRaw());
        cmd->SetViewport(viewport);
        cmd->SetScissor(scissor);
        cmd->BindVertexBuffer(0, vertexBuffer.GetRaw());
        cmd->BindIndexBuffer(indexBuffer.GetRaw());
        FE::List<HAL::ClearValueDesc> clearValues = { HAL::ClearValueDesc::CreateColorValue(FE::Colors::MediumAquamarine),
                                                      HAL::ClearValueDesc::CreateDepthStencilValue() };
        cmd->BeginRenderPass(renderPass.GetRaw(), framebuffer.GetRaw(), clearValues);
        cmd->DrawIndexed(meshAsset->IndexSize() / sizeof(FE::UInt32), 1, 0, 0, 0);
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