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
    auto assetLoader   = FE::MakeShared<HAL::ImageAssetLoader>();
    assetRegistry->LoadAssetsFromFile("../../Samples/Textures/FerrumAssetIndex");
    assetProvider->AttachRegistry(assetRegistry);
    assetManager->RegisterAssetLoader(static_pointer_cast<FE::Assets::IAssetLoader>(assetLoader));
    assetManager->AttachAssetProvider(static_pointer_cast<FE::Assets::IAssetProvider>(assetProvider));

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
    swapChainDesc.Queue              = graphicsQueue.Get();
    auto swapChain                   = device->CreateSwapChain(swapChainDesc);

    FE::Rc<HAL::IBuffer> indexBufferStaging, vertexBufferStaging;
    FE::Rc<HAL::IBuffer> vsConstantBuffer;
    FE::Rc<HAL::IBuffer> indexBuffer, vertexBuffer;
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
        vertexBufferStaging = device->CreateBuffer(HAL::BufferDesc{ vertexSize, HAL::BindFlags::None });
        vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        vertexBufferStaging->UpdateData(vertexData.Data());

        vertexBuffer = device->CreateBuffer(HAL::BufferDesc{ vertexSize, HAL::BindFlags::VertexBuffer });
        vertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }
    {
        FE::List<uint32_t> indexData = { 0, 2, 3, 3, 2, 1 };
        indexSize                      = indexData.Size() * sizeof(uint32_t);
        indexBufferStaging             = device->CreateBuffer(HAL::BufferDesc{ indexSize, HAL::BindFlags::None });
        indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        indexBufferStaging->UpdateData(indexData.Data());

        indexBuffer = device->CreateBuffer(HAL::BufferDesc{ indexSize, HAL::BindFlags::IndexBuffer });
        indexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }

    FE::Rc<HAL::IBuffer> textureStaging;
    FE::Rc<HAL::IImage> textureImage;
    {
        auto imageAsset = FE::Assets::Asset<HAL::ImageAssetStorage>(FE::Assets::AssetID("94FC6391-4656-4BE7-844D-8D87680A00F1"));
        imageAsset.LoadSync();

        textureStaging = device->CreateBuffer(HAL::BufferDesc{ imageAsset->Size(), HAL::BindFlags::None });
        textureStaging->AllocateMemory(HAL::MemoryType::HostVisible);
        textureStaging->UpdateData(imageAsset->Data());

        auto imageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::TransferWrite | HAL::ImageBindFlags::TransferRead
                                                   | HAL::ImageBindFlags::ShaderRead,
                                               imageAsset->Width(),
                                               imageAsset->Height(),
                                               HAL::Format::R8G8B8A8_SRGB,
                                               true);
        textureImage   = device->CreateImage(imageDesc);
        textureImage->AllocateMemory(HAL::MemoryType::DeviceLocal);
    }

    {
        FE::Vector3F constantData = { 0.3f, -0.4f, 0.0f };
        vsConstantBuffer          = device->CreateBuffer(HAL::BufferDesc{ sizeof(FE::Vector3F), HAL::BindFlags::ConstantBuffer });
        vsConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
        vsConstantBuffer->UpdateData(constantData.Data());
    }

    auto textureView = textureImage->CreateView(HAL::ImageAspectFlags::Color);
    {
        auto transferComplete = device->CreateFence(HAL::FenceState::Reset);
        auto commandBuffer    = device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics);
        commandBuffer->Begin();
        commandBuffer->CopyBuffers(vertexBufferStaging.Get(), vertexBuffer.Get(), HAL::BufferCopyRegion(vertexSize));
        commandBuffer->CopyBuffers(indexBufferStaging.Get(), indexBuffer.Get(), HAL::BufferCopyRegion(indexSize));

        HAL::ImageBarrierDesc barrier{};
        barrier.Image                        = textureImage.Get();
        barrier.SubresourceRange.AspectFlags = HAL::ImageAspectFlags::Color;
        barrier.StateAfter                   = HAL::ResourceState::TransferWrite;
        commandBuffer->ResourceTransitionBarriers({ barrier }, {});

        auto size = textureImage->GetDesc().ImageSize;
        commandBuffer->CopyBufferToImage(textureStaging.Get(), textureImage.Get(), HAL::BufferImageCopyRegion(size));

        barrier.StateAfter = HAL::ResourceState::TransferRead;
        commandBuffer->ResourceTransitionBarriers({ barrier }, {});

        auto mipCount    = static_cast<FE::uint16_t>(textureImage->GetDesc().MipSliceCount);
        auto textureSize = textureImage->GetDesc().ImageSize;
        for (FE::uint16_t i = 1; i < mipCount; ++i)
        {
            HAL::ImageBarrierDesc mipBarrier{};
            mipBarrier.Image                        = textureImage.Get();
            mipBarrier.SubresourceRange.AspectFlags = HAL::ImageAspectFlags::Color;
            mipBarrier.SubresourceRange.MinMipSlice = i;
            mipBarrier.StateAfter                   = HAL::ResourceState::TransferWrite;

            HAL::ImageBlitRegion blitRegion{};
            blitRegion.Source.Aspect   = HAL::ImageAspect::Color;
            blitRegion.Source.MipSlice = i - 1;

            blitRegion.SourceBounds[0] = {};
            blitRegion.SourceBounds[1] = { FE::int64_t(textureSize.Width >> (i - 1)), FE::int64_t(textureSize.Height >> (i - 1)), 1 };

            blitRegion.Dest.Aspect   = HAL::ImageAspect::Color;
            blitRegion.Dest.MipSlice = i;

            blitRegion.DestBounds[0] = {};
            blitRegion.DestBounds[1] = { FE::int64_t(textureSize.Width >> i), FE::int64_t(textureSize.Height >> i), 1 };

            commandBuffer->ResourceTransitionBarriers({ mipBarrier }, {});
            commandBuffer->BlitImage(textureImage.Get(), textureImage.Get(), blitRegion);

            mipBarrier.StateAfter = HAL::ResourceState::TransferRead;
            commandBuffer->ResourceTransitionBarriers({ mipBarrier }, {});
        }

        barrier.SubresourceRange = textureView->GetDesc().SubresourceRange;
        barrier.StateAfter       = HAL::ResourceState::ShaderResource;
        commandBuffer->ResourceTransitionBarriers({ barrier }, {});
        commandBuffer->End();
        graphicsQueue->SubmitBuffers({ commandBuffer.Get() }, { transferComplete.Get() }, HAL::SubmitFlags::None);
        transferComplete->WaitOnCPU();
    }

    auto textureSampler = device->CreateSampler(HAL::SamplerDesc{});

    auto compiler = device->CreateShaderCompiler();
    HAL::ShaderCompilerArgs psArgs{};
    psArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    psArgs.Stage      = HAL::ShaderStage::Pixel;
    psArgs.EntryPoint = "main";
    psArgs.FullPath   = "../../Samples/Textures/Shaders/PixelShader.hlsl";
    auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
    psArgs.SourceCode = psSource;
    auto psByteCode   = compiler->CompileShader(psArgs);

    auto pixelShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));

    HAL::ShaderCompilerArgs vsArgs{};
    vsArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
    vsArgs.Stage      = HAL::ShaderStage::Vertex;
    vsArgs.EntryPoint = "main";
    vsArgs.FullPath   = "../../Samples/Textures/Shaders/VertexShader.hlsl";
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

    HAL::DescriptorWriteSampler descriptorWriteSampler{ textureSampler.Get() };
    descriptorTable->Update(descriptorWriteSampler);
    HAL::DescriptorWriteImage descriptorWriteImage{ textureView.Get() };
    descriptorWriteImage.Binding = 1;
    descriptorTable->Update(descriptorWriteImage);
    HAL::DescriptorWriteBuffer descriptorWrite{ vsConstantBuffer.Get() };
    descriptorWrite.Binding = 2;
    descriptorWrite.Buffer  = vsConstantBuffer.Get();
    descriptorTable->Update(descriptorWrite);

    HAL::GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
                                   .AddBuffer(HAL::InputStreamRate::PerVertex)
                                   .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                   .AddAttribute(HAL::Format::R32G32_SFloat, "TEXCOORD")
                                   .Build()
                                   .Build();

    FE::List shaders{ pixelShader.Get(), vertexShader.Get() };
    FE::List descriptorTables{ descriptorTable.Get() };

    pipelineDesc.RenderPass       = renderPass.Get();
    pipelineDesc.SubpassIndex     = 0;
    pipelineDesc.ColorBlend       = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
    pipelineDesc.Shaders          = shaders;
    pipelineDesc.DescriptorTables = descriptorTables;
    pipelineDesc.Viewport         = viewport;
    pipelineDesc.Scissor          = scissor;
    pipelineDesc.Rasterization    = HAL::RasterizationState{};

    pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

    auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

    FE::List<FE::Rc<HAL::IFence>> fences;
    for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
    {
        fences.Push(device->CreateFence(HAL::FenceState::Signaled));
    }

    auto RTVs = swapChain->GetRTVs();
    FE::List<FE::Rc<HAL::IFramebuffer>> framebuffers;
    FE::List<FE::Rc<HAL::ICommandBuffer>> commandBuffers;
    for (size_t i = 0; i < swapChain->GetImageCount(); ++i)
    {
        HAL::FramebufferDesc framebufferDesc{};
        framebufferDesc.RenderPass        = renderPass.Get();
        framebufferDesc.RenderTargetViews = { RTVs[i] };
        framebufferDesc.Width             = scissor.Width();
        framebufferDesc.Height            = scissor.Height();
        auto framebuffer                  = framebuffers.Emplace(device->CreateFramebuffer(framebufferDesc));

        auto& cmd = commandBuffers.Emplace(device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
        cmd->Begin();
        cmd->BindGraphicsPipeline(pipeline.Get());
        cmd->BindDescriptorTables({ descriptorTable.Get() }, pipeline.Get());
        cmd->SetViewport(viewport);
        cmd->SetScissor(scissor);
        cmd->BindVertexBuffer(0, vertexBuffer.Get(), 0);
        cmd->BindIndexBuffer(indexBuffer.Get(), 0);
        cmd->BeginRenderPass(
            renderPass.Get(), framebuffer.Get(), { HAL::ClearValueDesc::CreateColorValue(FE::Colors::MediumAquamarine) });
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
