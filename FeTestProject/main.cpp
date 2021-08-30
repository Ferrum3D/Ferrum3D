#include <FeCore/Console/FeLog.h>
#include <FeCore/Math/Vector4.h>
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Instance/IInstance.h>
#include <FeGPU/Pipeline/InputLayoutBuilder.h>
#include <fstream>

struct Vertex
{
    FE::Float32 XYZ[3];
    FE::Float32 RGB[3];
};

FE::String ReadFile(const FE::String& path)
{
    std::ifstream stream(path.Data());
    FE_ASSERT(stream.good());
    stream.seekg(0, std::ios::end);
    auto len = stream.tellg();
    FE::String result(len, ' ');
    stream.seekg(0, std::ios::beg);
    stream.read(result.Data(), len);
    return result;
}

namespace HAL = FE::GPU;

int main()
{
    const int frameBufferCount = 3;
    FE::Env::CreateEnvironment();
    FE::GlobalAllocator<FE::HeapAllocator>::Init(FE::HeapAllocatorDesc{});
    {
        auto logger = FE::MakeShared<FE::Debug::ConsoleLogger>();
        FE_LOG_MESSAGE(
            "Running {} version {}.{}.{}", FE::StringSlice(FE::FerrumEngineName), FE::FerrumVersion.Major,
            FE::FerrumVersion.Minor, FE::FerrumVersion.Patch);

        HAL::InstanceDesc desc{};
        auto instance      = HAL::CreateGraphicsAPIInstance(desc, HAL::GraphicsAPI::Vulkan);
        auto adapter       = instance->GetAdapters()[0];
        auto device        = adapter->CreateDevice();
        auto graphicsQueue = device->GetCommandQueue(HAL::CommandQueueClass::Graphics);
        auto transferQueue = device->GetCommandQueue(HAL::CommandQueueClass::Transfer);
        auto compiler      = device->CreateShaderCompiler();

        auto window   = device->CreateWindow(HAL::WindowDesc{800, 600, "Test project"});
        auto viewport = window->CreateViewport();
        auto scissor  = window->CreateScissor();

        HAL::SwapChainDesc swapChainDesc{};
        swapChainDesc.ImageCount         = frameBufferCount;
        swapChainDesc.ImageWidth         = scissor.Width();
        swapChainDesc.ImageHeight        = scissor.Height();
        swapChainDesc.NativeWindowHandle = window->GetNativeHandle();
        swapChainDesc.Queue              = graphicsQueue.GetRaw();
        swapChainDesc.VerticalSync       = true;
        auto swapChain                   = device->CreateSwapChain(swapChainDesc);

        FE::RefCountPtr<HAL::IBuffer> indexBufferStaging, vertexBufferStaging, constantBuffer;
        FE::RefCountPtr<HAL::IBuffer> indexBuffer, vertexBuffer;
        FE::UInt64 vertexSize, indexSize;
        {
            // clang-format off
            FE::Vector<Vertex> vertexData = {
                { {-0.5f, -0.5f, 0.0f}, {1.0f, 0.1f, 0.1f} },
                { {+0.5f, +0.5f, 0.0f}, {0.1f, 1.0f, 0.1f} },
                { {+0.5f, -0.5f, 0.0f}, {0.1f, 0.1f, 1.0f} },
                { {-0.5f, +0.5f, 0.0f}, {1.0f, 1.0f, 0.1f} }
            };
            // clang-format on
            vertexSize          = vertexData.size() * sizeof(Vertex);
            vertexBufferStaging = device->CreateBuffer(HAL::BindFlags::None, vertexSize);
            vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            void* map = vertexBufferStaging->Map(0);
            memcpy(map, vertexData.data(), vertexSize);
            vertexBufferStaging->Unmap();

            vertexBuffer = device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexSize);
            vertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            FE::Vector<FE::UInt32> indexData = { 0, 2, 3, 3, 2, 1 };
            indexSize                        = indexData.size() * sizeof(FE::UInt32);
            indexBufferStaging               = device->CreateBuffer(HAL::BindFlags::None, indexSize);
            indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            void* map = indexBufferStaging->Map(0);
            memcpy(map, indexData.data(), indexSize);
            indexBufferStaging->Unmap();

            indexBuffer = device->CreateBuffer(HAL::BindFlags::IndexBuffer, indexSize);
            indexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            FE::float4 constantData = { 1.0f, 1.0f, 1.0f, 1.0f };
            constantData *= 0.7f;
            constantBuffer = device->CreateBuffer(HAL::BindFlags::ConstantBuffer, sizeof(FE::float4));
            constantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
            void* map = constantBuffer->Map(0);
            memcpy(map, constantData.Data(), sizeof(FE::float4));
            constantBuffer->Unmap();
        }

        {
            auto transferComplete = device->CreateFence(HAL::FenceState::Reset);
            auto copyCmdBuffer    = device->CreateCommandBuffer(HAL::CommandQueueClass::Transfer);
            copyCmdBuffer->Begin();
            copyCmdBuffer->CopyBuffers(vertexBufferStaging.GetRaw(), vertexBuffer.GetRaw(), HAL::BufferCopyRegion(vertexSize));
            copyCmdBuffer->CopyBuffers(indexBufferStaging.GetRaw(), indexBuffer.GetRaw(), HAL::BufferCopyRegion(indexSize));
            copyCmdBuffer->End();
            transferQueue->SubmitBuffers({ copyCmdBuffer }, { transferComplete }, HAL::SubmitFlags::None);
            transferComplete->WaitOnCPU();
        }

        HAL::ShaderCompilerArgs psArgs{};
        psArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        psArgs.Stage      = HAL::ShaderStage::Pixel;
        psArgs.EntryPoint = "main";
        psArgs.FullPath   = "Assets/Shaders/PixelShader.hlsl";
        auto psSource     = ReadFile(psArgs.FullPath);
        psArgs.SourceCode = psSource;
        auto psByteCode   = compiler->CompileShader(psArgs);

        HAL::ShaderModuleDesc pixelDesc;
        pixelDesc.EntryPoint   = "main";
        pixelDesc.ByteCode     = psByteCode.data();
        pixelDesc.ByteCodeSize = psByteCode.size();
        pixelDesc.Stage        = HAL::ShaderStage::Pixel;
        auto pixelShader       = device->CreateShaderModule(pixelDesc);

        HAL::ShaderCompilerArgs vsArgs{};
        vsArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        vsArgs.Stage      = HAL::ShaderStage::Vertex;
        vsArgs.EntryPoint = "main";
        vsArgs.FullPath   = "Assets/Shaders/VertexShader.hlsl";
        auto vsSource     = ReadFile(vsArgs.FullPath);
        vsArgs.SourceCode = vsSource;
        auto vsByteCode   = compiler->CompileShader(vsArgs);

        HAL::ShaderModuleDesc vertexDesc;
        vertexDesc.EntryPoint   = "main";
        vertexDesc.ByteCode     = vsByteCode.data();
        vertexDesc.ByteCodeSize = vsByteCode.size();
        vertexDesc.Stage        = HAL::ShaderStage::Vertex;
        auto vertexShader       = device->CreateShaderModule(vertexDesc);

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
        descriptorHeapDesc.MaxSets = 1;
        descriptorHeapDesc.Sizes   = { HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer) };
        auto descriptorHeap        = device->CreateDescriptorHeap(descriptorHeapDesc);

        HAL::DescriptorDesc descriptorDesc{};
        descriptorDesc.Stage        = HAL::ShaderStageFlags::Pixel;
        descriptorDesc.ResourceType = HAL::ShaderResourceType::ConstantBuffer;
        descriptorDesc.Count        = 1;
        auto descriptorTable        = descriptorHeap->AllocateDescriptorTable({ descriptorDesc });

        HAL::DescriptorWriteBuffer write{};
        write.Buffer = constantBuffer.GetRaw();
        descriptorTable->Update(write);

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder{}
                                       .AddBuffer(HAL::InputStreamRate::PerVertex)
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "COLOR")
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

        FE::Vector<FE::RefCountPtr<HAL::IFence>> fences;
        for (size_t i = 0; i < swapChain->GetDesc().FrameCount; ++i)
        {
            fences.push_back(device->CreateFence(HAL::FenceState::Signaled));
        }

        auto RTVs = swapChain->GetRTVs();
        FE::Vector<FE::RefCountPtr<HAL::IFramebuffer>> framebuffers;
        FE::Vector<FE::RefCountPtr<HAL::ICommandBuffer>> commandBuffers;
        for (size_t i = 0; i < frameBufferCount; ++i)
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
            cmd->BindDescriptorTables({ descriptorTable.GetRaw() }, pipeline.GetRaw());
            cmd->SetViewport(viewport);
            cmd->SetScissor(scissor);
            cmd->BindVertexBuffer(0, vertexBuffer.GetRaw());
            cmd->BindIndexBuffer(indexBuffer.GetRaw());
            cmd->BeginRenderPass(renderPass.GetRaw(), framebuffer.GetRaw(), HAL::ClearValueDesc{ { 0.1f, 0.1f, 0.7f, 1 } });
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
            graphicsQueue->SubmitBuffers({ commandBuffers[imageIndex] }, fences[frameIndex], HAL::SubmitFlags::FrameBeginEnd);
            swapChain->Present();
        }

        device->WaitIdle();
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
}
