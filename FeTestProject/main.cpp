/*
 * Copyright 2021 Nikita Dubovikov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/IO/StdoutStream.h>
#include <FeCore/Jobs/JobScheduler.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <GPU/OsmiumGPU.h>
#include <chrono>

struct Vertex
{
    [[maybe_unused]] FE::Float32 XYZ[3];
    [[maybe_unused]] FE::Float32 RGB[3];
};

static FE::AtomicInt32 JobCounter = 0;

class TestJob : public FE::Job
{
public:
    int id;

    explicit TestJob(int i, FE::JobPriority priority = FE::JobPriority::Normal)
        : FE::Job(priority, false)
        , id(i)
    {
    }

protected:
    void Execute(const FE::JobExecutionContext&) override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        FE::Interlocked::Increment(JobCounter);
    }
};

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

        auto eventBus     = FE::MakeShared<FE::EventBus<FE::FrameEvents>>();
        auto jobScheduler = FE::MakeShared<FE::JobScheduler>(std::thread::hardware_concurrency() - 1);

        FE::IO::StdoutStream stdoutStream;
        {
            FE::String b = "Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘\n";
            stdoutStream.WriteFromBuffer(b.Data(), b.Size());
        }

        FE::Vector<TestJob> testJobs;
        testJobs.reserve(16 * 1024);
        for (int i = 0; i < 16 * 1024; ++i)
        {
            testJobs.emplace_back(TestJob(i)).Schedule();
        }
        testJobs.emplace_back(TestJob(999999, FE::JobPriority::Highest)).Schedule();

        FE_LOG_MESSAGE("Counter: {}", JobCounter);

        FE::DynamicLibrary osmiumLib("OsmiumGPU");
        auto attachEnvironment = osmiumLib.GetFunction<HAL::AttachEnvironmentProc>("AttachEnvironment");
        attachEnvironment(&FE::Env::GetEnvironment());
        auto createGraphicsAPIInstance = osmiumLib.GetFunction<HAL::CreateGraphicsAPIInstanceProc>("CreateGraphicsAPIInstance");

        auto instance = FE::Shared<HAL::IInstance>(createGraphicsAPIInstance(HAL::InstanceDesc{}, HAL::GraphicsAPI::Vulkan));
        instance->ReleaseStrongRef();

        auto adapter       = instance->GetAdapters()[0];
        auto device        = adapter->CreateDevice();
        auto graphicsQueue = device->GetCommandQueue(HAL::CommandQueueClass::Graphics);
        auto transferQueue = device->GetCommandQueue(HAL::CommandQueueClass::Transfer);
        auto compiler      = device->CreateShaderCompiler();

        auto window   = device->CreateWindow(HAL::WindowDesc{ 800, 600, "Test project" });
        auto viewport = window->CreateViewport();
        auto scissor  = window->CreateScissor();

        HAL::SwapChainDesc swapChainDesc{};
        swapChainDesc.ImageCount         = frameBufferCount;
        swapChainDesc.ImageWidth         = scissor.Width();
        swapChainDesc.ImageHeight        = scissor.Height();
        swapChainDesc.NativeWindowHandle = window->GetNativeHandle();
        swapChainDesc.Queue              = graphicsQueue.GetRaw();
        swapChainDesc.VerticalSync       = false;
        auto swapChain                   = device->CreateSwapChain(swapChainDesc);

        FE::Shared<HAL::IBuffer> indexBufferStaging, vertexBufferStaging, constantBuffer;
        FE::Shared<HAL::IBuffer> indexBuffer, vertexBuffer;
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
            vertexBufferStaging->UpdateData(vertexData.data());

            vertexBuffer = device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexSize);
            vertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            FE::Vector<FE::UInt32> indexData = { 0, 2, 3, 3, 2, 1 };
            indexSize                        = indexData.size() * sizeof(FE::UInt32);
            indexBufferStaging               = device->CreateBuffer(HAL::BindFlags::None, indexSize);
            indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            indexBufferStaging->UpdateData(indexData.data());

            indexBuffer = device->CreateBuffer(HAL::BindFlags::IndexBuffer, indexSize);
            indexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            FE::Color constantData = FE::Colors::White;
            constantData *= 0.7f;
            constantBuffer = device->CreateBuffer(HAL::BindFlags::ConstantBuffer, sizeof(FE::Vector4F));
            constantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
            constantBuffer->UpdateData(constantData.Data());
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

        HAL::ShaderCompilerArgs psArgs{};
        psArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        psArgs.Stage      = HAL::ShaderStage::Pixel;
        psArgs.EntryPoint = "main";
        psArgs.FullPath   = "../Assets/TestProject/Shaders/PixelShader.hlsl";
        auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
        psArgs.SourceCode = psSource;
        auto psByteCode   = compiler->CompileShader(psArgs);

        auto pixelShader = device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));

        HAL::ShaderCompilerArgs vsArgs{};
        vsArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        vsArgs.Stage      = HAL::ShaderStage::Vertex;
        vsArgs.EntryPoint = "main";
        vsArgs.FullPath   = "../Assets/TestProject/Shaders/VertexShader.hlsl";
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
        descriptorHeapDesc.MaxSets = 1;
        descriptorHeapDesc.Sizes   = { HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer) };
        auto descriptorHeap        = device->CreateDescriptorHeap(descriptorHeapDesc);

        HAL::DescriptorDesc descriptorDesc(HAL::ShaderResourceType::ConstantBuffer, HAL::ShaderStageFlags::Pixel, 1);
        auto descriptorTable = descriptorHeap->AllocateDescriptorTable({ descriptorDesc });

        descriptorTable->Update(HAL::DescriptorWriteBuffer(constantBuffer.GetRaw()));

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
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
            cmd->BeginRenderPass(renderPass.GetRaw(), framebuffer.GetRaw(), HAL::ClearValueDesc{ FE::Colors::Coral });
            cmd->DrawIndexed(6, 1, 0, 0, 0);
            cmd->EndRenderPass();
            cmd->End();
        }

        FE::FrameEventArgs frameEventArgs{};
        auto ts          = std::chrono::high_resolution_clock::now();
        FE::Int16 frames = 0;
        while (!window->CloseRequested())
        {
            auto getDelta = [](std::chrono::high_resolution_clock::time_point ts) {
                auto now = std::chrono::high_resolution_clock::now();
                auto ms  = std::chrono::duration_cast<std::chrono::microseconds>(now - ts).count();
                return static_cast<FE::Float32>(ms) / 1'000'000;
            };
            auto frameIndex          = swapChain->GetCurrentFrameIndex();
            frameEventArgs.DeltaTime = getDelta(ts);
            eventBus->SendEvent(&FE::FrameEvents::OnFrameStart, frameEventArgs);

            frameEventArgs.DeltaTime = getDelta(ts);
            eventBus->SendEvent(&FE::FrameEvents::OnUpdate, frameEventArgs);
            frameEventArgs.DeltaTime = getDelta(ts);
            eventBus->SendEvent(&FE::FrameEvents::OnLateUpdate, frameEventArgs);

            fences[frameIndex]->WaitOnCPU();
            window->PollEvents();
            auto imageIndex = swapChain->GetCurrentImageIndex();
            fences[swapChain->GetCurrentFrameIndex()]->Reset();
            graphicsQueue->SubmitBuffers(
                { commandBuffers[imageIndex].GetRaw() }, fences[frameIndex], HAL::SubmitFlags::FrameBeginEnd);
            swapChain->Present();

            if ((frames = (frames + 1) % 1000) == 0)
            {
                auto delta = getDelta(ts);
                FE_LOG_MESSAGE("Counter: {}", JobCounter);
                FE_LOG_MESSAGE("Delta: {}sec, FPS: {}", delta, 1.0f / delta);
            }
            frameEventArgs.DeltaTime = getDelta(ts);
            eventBus->SendEvent(&FE::FrameEvents::OnFrameEnd, frameEventArgs);
            ts = std::chrono::high_resolution_clock::now();
        }

        device->WaitIdle();
    }
    FE::GlobalAllocator<FE::HeapAllocator>::Destroy();
}
