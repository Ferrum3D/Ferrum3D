#include <FeCore/Assets/Asset.h>
#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/AssetProviderDev.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Images/ImageAssetStorage.h>
#include <OsGPU/OsmiumGPU.h>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
    [[maybe_unused]] FE::Float32 UV[2];
};

namespace HAL = FE::Osmium;
using FE::static_pointer_cast;

inline constexpr const char* ExampleName = "Ferrum3D - Textures";

void RunExample()
{
    auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();

    auto assetManager  = FE::MakeShared<FE::Assets::AssetManager>();
    auto assetProvider = FE::MakeShared<FE::Assets::AssetProviderDev>();
    auto assetRegistry = FE::MakeShared<FE::Assets::AssetRegistry>();
    auto assetLoader   = FE::MakeShared<RHI::ImageAssetLoader>();
    assetRegistry->LoadAssetsFromFile("../../Samples/Textures/FerrumAssetIndex");
    assetProvider->AttachRegistry(assetRegistry);
    assetManager->RegisterAssetLoader(static_pointer_cast<FE::Assets::IAssetLoader>(assetLoader));
    assetManager->AttachAssetProvider(static_pointer_cast<FE::Assets::IAssetProvider>(assetProvider));

    FE::DynamicLibrary osmiumLib;
    osmiumLib.LoadFrom("OsGPU");
    auto attachEnvironment = osmiumLib.GetFunction<RHI::AttachEnvironmentProc>("AttachEnvironment");
    attachEnvironment(&FE::Env::GetEnvironment());
    auto createGraphicsAPIInstance = osmiumLib.GetFunction<RHI::CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

    auto instance =
        FE::Rc<RHI::IInstance>(createGraphicsAPIInstance(RHI::InstanceDesc{ ExampleName }, RHI::GraphicsAPI::Vulkan));
    instance->ReleaseStrongRef();
    auto adapter       = instance->GetAdapters().Front();
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
    FE::Rc<RHI::IBuffer> vsConstantBuffer;
    FE::Rc<RHI::IBuffer> indexBuffer, vertexBuffer;
    uint64_t vertexSize, indexSize;
    {
        // clang-format off
        FE::List<Vertex> vertexData = {
            { {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f} },
            { {+0.5f, +0.5f, 0.0f}, {0.0f, 1.0f} },
            { {+0.5f, -0.5f, 0.0f}, {0.0f, 0.0f} },
            { {-0.5f, +0.5f, 0.0f}, {1.0f, 1.0f} }
        };
        // clang-format on
        vertexSize          = vertexData.Size() * sizeof(Vertex);
        vertexBufferStaging = device->CreateBuffer(RHI::BufferDesc{ vertexSize, RHI::BindFlags::None });
        vertexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
        vertexBufferStaging->UpdateData(vertexData.Data());

        vertexBuffer = device->CreateBuffer(RHI::BufferDesc{ vertexSize, RHI::BindFlags::VertexBuffer });
        vertexBuffer->AllocateMemory(RHI::MemoryType::kDeviceLocal);
    }
    {
        FE::List<uint32_t> indexData = { 0, 2, 3, 3, 2, 1 };
        indexSize                      = indexData.Size() * sizeof(uint32_t);
        indexBufferStaging             = device->CreateBuffer(RHI::BufferDesc{ indexSize, RHI::BindFlags::None });
        indexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
        indexBufferStaging->UpdateData(indexData.Data());

        indexBuffer = device->CreateBuffer(RHI::BufferDesc{ indexSize, RHI::BindFlags::IndexBuffer });
        indexBuffer->AllocateMemory(RHI::MemoryType::kDeviceLocal);
    }

    FE::Rc<RHI::IBuffer> textureStaging;
    FE::Rc<RHI::Image> textureImage;
    {
        auto imageAsset = FE::Assets::Asset<RHI::ImageAssetStorage>(FE::Assets::AssetID("94FC6391-4656-4BE7-844D-8D87680A00F1"));
        imageAsset.LoadSync();

        textureStaging = device->CreateBuffer(RHI::BufferDesc{ imageAsset->Size(), RHI::BindFlags::None });
        textureStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
        textureStaging->UpdateData(imageAsset->Data());

        auto imageDesc = RHI::ImageDesc::Img2D(RHI::ImageBindFlags::kTransferWrite | RHI::ImageBindFlags::kTransferRead
                                                   | RHI::ImageBindFlags::kShaderRead,
                                               imageAsset->Width(),
                                               imageAsset->Height(),
                                               RHI::Format::R8G8B8A8_SRGB,
                                               true);
        textureImage   = device->CreateImage(imageDesc);
        textureImage->AllocateMemory(RHI::MemoryType::kDeviceLocal);
    }

    {
        FE::Vector3F constantData = { 0.3f, -0.4f, 0.0f };
        vsConstantBuffer          = device->CreateBuffer(RHI::BufferDesc{ sizeof(FE::Vector3F), RHI::BindFlags::ConstantBuffer });
        vsConstantBuffer->AllocateMemory(RHI::MemoryType::kHostVisible);
        vsConstantBuffer->UpdateData(constantData.Data());
    }

    auto textureView = textureImage->CreateView(RHI::ImageAspectFlags::kColor);
    {
        auto transferComplete = device->CreateFence(RHI::FenceState::Reset);
        auto commandBuffer    = device->CreateCommandBuffer(RHI::CommandQueueClass::Graphics);
        commandBuffer->Begin();
        commandBuffer->CopyBuffers(vertexBufferStaging.Get(), vertexBuffer.Get(), RHI::BufferCopyRegion(vertexSize));
        commandBuffer->CopyBuffers(indexBufferStaging.Get(), indexBuffer.Get(), RHI::BufferCopyRegion(indexSize));

        RHI::ImageBarrierDesc barrier{};
        barrier.Image                        = textureImage.Get();
        barrier.SubresourceRange.AspectFlags = RHI::ImageAspectFlags::kColor;
        barrier.StateAfter                   = RHI::ResourceState::kTransferWrite;
        commandBuffer->ResourceTransitionBarriers({ barrier }, {});

        auto size = textureImage->GetDesc().ImageSize;
        commandBuffer->CopyBufferToImage(textureStaging.Get(), textureImage.Get(), RHI::BufferImageCopyRegion(size));

        barrier.StateAfter = RHI::ResourceState::kTransferRead;
        commandBuffer->ResourceTransitionBarriers({ barrier }, {});

        auto mipCount    = static_cast<FE::uint16_t>(textureImage->GetDesc().MipSliceCount);
        auto textureSize = textureImage->GetDesc().ImageSize;
        for (FE::uint16_t i = 1; i < mipCount; ++i)
        {
            RHI::ImageBarrierDesc mipBarrier{};
            mipBarrier.Image                        = textureImage.Get();
            mipBarrier.SubresourceRange.AspectFlags = RHI::ImageAspectFlags::kColor;
            mipBarrier.SubresourceRange.MinMipSlice = i;
            mipBarrier.StateAfter                   = RHI::ResourceState::kTransferWrite;

            RHI::ImageBlitRegion blitRegion{};
            blitRegion.Source.Aspect   = RHI::ImageAspect::kColor;
            blitRegion.Source.MipSlice = i - 1;

            blitRegion.SourceBounds[0] = {};
            blitRegion.SourceBounds[1] = { FE::int64_t(textureSize.Width >> (i - 1)), FE::int64_t(textureSize.Height >> (i - 1)), 1 };

            blitRegion.Dest.Aspect   = RHI::ImageAspect::kColor;
            blitRegion.Dest.MipSlice = i;

            blitRegion.DestBounds[0] = {};
            blitRegion.DestBounds[1] = { FE::int64_t(textureSize.Width >> i), FE::int64_t(textureSize.Height >> i), 1 };

            commandBuffer->ResourceTransitionBarriers({ mipBarrier }, {});
            commandBuffer->BlitImage(textureImage.Get(), textureImage.Get(), blitRegion);

            mipBarrier.StateAfter = RHI::ResourceState::kTransferRead;
            commandBuffer->ResourceTransitionBarriers({ mipBarrier }, {});
        }

        barrier.SubresourceRange = textureView->GetDesc().SubresourceRange;
        barrier.StateAfter       = RHI::ResourceState::kShaderResource;
        commandBuffer->ResourceTransitionBarriers({ barrier }, {});
        commandBuffer->End();
        graphicsQueue->SubmitBuffers({ commandBuffer.Get() }, { transferComplete.Get() }, RHI::SubmitFlags::None);
        transferComplete->WaitOnCPU();
    }

    auto textureSampler = device->CreateSampler(RHI::SamplerDesc{});

    auto compiler = device->CreateShaderCompiler();
    RHI::ShaderCompilerArgs psArgs{};
    psArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
    psArgs.Stage      = RHI::ShaderStage::kPixel;
    psArgs.EntryPoint = "main";
    psArgs.FullPath   = "../../Samples/Textures/Shaders/PixelShader.hlsl";
    auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
    psArgs.SourceCode = psSource;
    auto psByteCode   = compiler->CompileShader(psArgs);

    auto pixelShader = device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kPixel, psByteCode));

    RHI::ShaderCompilerArgs vsArgs{};
    vsArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
    vsArgs.Stage      = RHI::ShaderStage::kVertex;
    vsArgs.EntryPoint = "main";
    vsArgs.FullPath   = "../../Samples/Textures/Shaders/VertexShader.hlsl";
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
    descriptorHeapDesc.MaxTables = 1;
    descriptorHeapDesc.Sizes     = { RHI::DescriptorSize(1, RHI::ShaderResourceType::Sampler),
                                     RHI::DescriptorSize(1, RHI::ShaderResourceType::TextureSRV),
                                     RHI::DescriptorSize(1, RHI::ShaderResourceType::ConstantBuffer) };
    auto descriptorHeap          = device->CreateDescriptorHeap(descriptorHeapDesc);

    RHI::DescriptorDesc psSamplerDescriptorDesc(RHI::ShaderResourceType::Sampler, RHI::ShaderStageFlags::kPixel, 1);
    RHI::DescriptorDesc psTextureDescriptorDesc(RHI::ShaderResourceType::TextureSRV, RHI::ShaderStageFlags::kPixel, 1);
    RHI::DescriptorDesc vsDescriptorDesc(RHI::ShaderResourceType::ConstantBuffer, RHI::ShaderStageFlags::kVertex, 1);
    auto descriptorTable =
        descriptorHeap->AllocateDescriptorTable({ psSamplerDescriptorDesc, psTextureDescriptorDesc, vsDescriptorDesc });

    RHI::DescriptorWriteSampler descriptorWriteSampler{ textureSampler.Get() };
    descriptorTable->Update(descriptorWriteSampler);
    RHI::DescriptorWriteImage descriptorWriteImage{ textureView.Get() };
    descriptorWriteImage.Binding = 1;
    descriptorTable->Update(descriptorWriteImage);
    RHI::DescriptorWriteBuffer descriptorWrite{ vsConstantBuffer.Get() };
    descriptorWrite.Binding = 2;
    descriptorWrite.Buffer  = vsConstantBuffer.Get();
    descriptorTable->Update(descriptorWrite);

    RHI::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = RHI::InputLayoutBuilder(RHI::PrimitiveTopology::kTriangleList)
                                   .AddBuffer(RHI::InputStreamRate::kPerVertex)
                                   .AddAttribute(RHI::Format::R32G32B32_SFloat, "POSITION")
                                   .AddAttribute(RHI::Format::R32G32_SFloat, "TEXCOORD")
                                   .Build()
                                   .Build();

    FE::List shaders{ pixelShader.Get(), vertexShader.Get() };
    FE::List descriptorTables{ descriptorTable.Get() };

    pipelineDesc.RenderPass       = renderPass.Get();
    pipelineDesc.SubpassIndex     = 0;
    pipelineDesc.ColorBlend       = RHI::ColorBlendState({ RHI::TargetColorBlending{} });
    pipelineDesc.Shaders          = shaders;
    pipelineDesc.DescriptorTables = descriptorTables;
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
        cmd->BindVertexBuffer(0, vertexBuffer.Get(), 0);
        cmd->BindIndexBuffer(indexBuffer.Get(), 0);
        cmd->BeginRenderPass(
            renderPass.Get(), framebuffer.Get(), { RHI::ClearValueDesc::CreateColorValue(FE::Colors::MediumAquamarine) });
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
            { commandBuffers[imageIndex].Get() }, fences[frameIndex].Get(), RHI::SubmitFlags::FrameBeginEnd);
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
