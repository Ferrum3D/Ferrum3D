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
#include <FeCore/Framework/ApplicationFramework.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/IO/StdoutStream.h>
#include <FeCore/Jobs/JobScheduler.h>
#include <FeCore/Math/Colors.h>
#include <OsGPU/OsmiumGPU.h>
#include <OsGPU/OsmiumGPUModule.h>
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

namespace HAL = FE::Osmium;

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

    FE::Shared<HAL::IDescriptorHeap> m_DescriptorHeap;
    FE::Shared<HAL::IDescriptorTable> m_DescriptorTable;

    FE::Shared<HAL::IShaderCompiler> m_Compiler;
    FE::Shared<HAL::IShaderModule> m_PixelShader;
    FE::Shared<HAL::IShaderModule> m_VertexShader;

    FE::Shared<HAL::IBuffer> m_ConstantBuffer;
    FE::Shared<HAL::IBuffer> m_IndexBuffer, m_VertexBuffer;

    FE::Shared<HAL::IWindow> m_Window;
    HAL::Viewport m_Viewport;
    HAL::Scissor m_Scissor;

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

    void Tick(const FE::FrameEventArgs& frameEventArgs) override
    {
        auto frameIndex = m_SwapChain->GetCurrentFrameIndex();
        auto delta      = frameEventArgs.DeltaTime;

        m_Fences[frameIndex]->WaitOnCPU();
        m_Window->PollEvents();
        auto imageIndex = m_SwapChain->GetCurrentImageIndex();
        m_Fences[m_SwapChain->GetCurrentFrameIndex()]->Reset();
        m_GraphicsQueue->SubmitBuffers(
            { m_CommandBuffers[imageIndex].GetRaw() }, m_Fences[frameIndex], HAL::SubmitFlags::FrameBeginEnd);
        m_SwapChain->Present();

        if ((frameEventArgs.FrameIndex + 1) % 1000 == 0)
        {
            FE_LOG_MESSAGE("Counter: {}", JobCounter);
            FE_LOG_MESSAGE("Delta: {}sec, FPS: {}", delta, 1.0f / delta);
        }
    }

    void GetFrameworkDependencies(FE::List<FE::Shared<FE::IFrameworkFactory>>& dependencies) override
    {
        dependencies.Push(HAL::OsmiumGPUModule::CreateFactory());
    }

public:
    FE_CLASS_RTTI(TestApplication, "04E7E607-FBE9-42C8-93E2-87E198AA1905");

    ~TestApplication() override
    {
        m_Device->WaitIdle();
        FE::SharedInterface<HAL::OsmiumGPUModule>::ReleaseFromCurrentModule();
    }

    void Initialize(const FE::ApplicationDesc& desc) override
    {
        ApplicationFramework::Initialize(desc);
        auto module = FE::SharedInterface<HAL::OsmiumGPUModule>::Get();
        module->Initialize(HAL::OsmiumGPUModuleDesc("Ferrum3D - Test", HAL::GraphicsAPI::Vulkan));

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

        FE_LOG_MESSAGE(
            "Running {} version {}.{}.{}", FE::StringSlice(FE::FerrumEngineName), FE::FerrumVersion.Major,
            FE::FerrumVersion.Minor, FE::FerrumVersion.Patch);
        m_Instance      = module->CreateInstance();
        m_Adapter       = m_Instance->GetAdapters()[0];
        m_Device        = m_Adapter->CreateDevice();
        m_GraphicsQueue = m_Device->GetCommandQueue(HAL::CommandQueueClass::Graphics);
        m_TransferQueue = m_Device->GetCommandQueue(HAL::CommandQueueClass::Transfer);
        m_Compiler      = m_Device->CreateShaderCompiler();
        m_Window        = m_Device->CreateWindow(HAL::WindowDesc{ Desc.WindowWidth, Desc.WindowHeight, "Test project" });
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

        FE::Shared<HAL::IBuffer> indexBufferStaging, vertexBufferStaging;
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
            vertexBufferStaging = m_Device->CreateBuffer(HAL::BindFlags::None, vertexSize);
            vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            vertexBufferStaging->UpdateData(vertexData.data());

            m_VertexBuffer = m_Device->CreateBuffer(HAL::BindFlags::VertexBuffer, vertexSize);
            m_VertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            FE::Vector<FE::UInt32> indexData = { 0, 2, 3, 3, 2, 1 };
            indexSize                        = indexData.size() * sizeof(FE::UInt32);
            indexBufferStaging               = m_Device->CreateBuffer(HAL::BindFlags::None, indexSize);
            indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            indexBufferStaging->UpdateData(indexData.data());

            m_IndexBuffer = m_Device->CreateBuffer(HAL::BindFlags::IndexBuffer, indexSize);
            m_IndexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            FE::Color constantData = FE::Colors::White;
            constantData *= 0.7f;
            m_ConstantBuffer = m_Device->CreateBuffer(HAL::BindFlags::ConstantBuffer, sizeof(FE::Vector4F));
            m_ConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
            m_ConstantBuffer->UpdateData(constantData.Data());
        }

        {
            auto transferComplete = m_Device->CreateFence(HAL::FenceState::Reset);
            auto copyCmdBuffer    = m_Device->CreateCommandBuffer(HAL::CommandQueueClass::Transfer);
            copyCmdBuffer->Begin();
            copyCmdBuffer->CopyBuffers(vertexBufferStaging.GetRaw(), m_VertexBuffer.GetRaw(), HAL::BufferCopyRegion(vertexSize));
            copyCmdBuffer->CopyBuffers(indexBufferStaging.GetRaw(), m_IndexBuffer.GetRaw(), HAL::BufferCopyRegion(indexSize));
            copyCmdBuffer->End();
            m_TransferQueue->SubmitBuffers({ copyCmdBuffer.GetRaw() }, { transferComplete }, HAL::SubmitFlags::None);
            transferComplete->WaitOnCPU();
        }

        HAL::ShaderCompilerArgs psArgs{};
        psArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        psArgs.Stage      = HAL::ShaderStage::Pixel;
        psArgs.EntryPoint = "main";
        psArgs.FullPath   = "../../FerrumTestProject/Shaders/PixelShader.hlsl";
        auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
        psArgs.SourceCode = psSource;
        auto psByteCode   = m_Compiler->CompileShader(psArgs);

        m_PixelShader = m_Device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));

        HAL::ShaderCompilerArgs vsArgs{};
        vsArgs.Version    = HAL::HLSLShaderVersion{ 6, 1 };
        vsArgs.Stage      = HAL::ShaderStage::Vertex;
        vsArgs.EntryPoint = "main";
        vsArgs.FullPath   = "../../FerrumTestProject/Shaders/VertexShader.hlsl";
        auto vsSource     = FE::IO::File::ReadAllText(vsArgs.FullPath);
        vsArgs.SourceCode = vsSource;
        auto vsByteCode   = m_Compiler->CompileShader(vsArgs);

        m_VertexShader = m_Device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Vertex, vsByteCode));

        HAL::RenderPassDesc renderPassDesc{};

        HAL::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format       = m_SwapChain->GetDesc().Format;
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

        m_RenderPass = m_Device->CreateRenderPass(renderPassDesc);

        HAL::DescriptorHeapDesc descriptorHeapDesc{};
        descriptorHeapDesc.MaxTables = 1;
        descriptorHeapDesc.Sizes     = { HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer) };
        m_DescriptorHeap             = m_Device->CreateDescriptorHeap(descriptorHeapDesc);

        HAL::DescriptorDesc descriptorDesc(HAL::ShaderResourceType::ConstantBuffer, HAL::ShaderStageFlags::Pixel, 1);
        m_DescriptorTable = m_DescriptorHeap->AllocateDescriptorTable({ descriptorDesc });

        m_DescriptorTable->Update(HAL::DescriptorWriteBuffer(m_ConstantBuffer.GetRaw()));

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
                                       .AddBuffer(HAL::InputStreamRate::PerVertex)
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "COLOR")
                                       .Build()
                                       .Build();

        pipelineDesc.RenderPass       = m_RenderPass;
        pipelineDesc.SubpassIndex     = 0;
        pipelineDesc.ColorBlend       = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
        pipelineDesc.Shaders          = { m_PixelShader, m_VertexShader };
        pipelineDesc.DescriptorTables = { m_DescriptorTable };
        pipelineDesc.Viewport         = m_Viewport;
        pipelineDesc.Scissor          = m_Scissor;
        pipelineDesc.Rasterization    = HAL::RasterizationState{};

        pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

        m_Pipeline = m_Device->CreateGraphicsPipeline(pipelineDesc);

        for (size_t i = 0; i < m_SwapChain->GetDesc().FrameCount; ++i)
        {
            m_Fences.Push(m_Device->CreateFence(HAL::FenceState::Signaled));
        }

        m_RTVs = m_SwapChain->GetRTVs();
        for (size_t i = 0; i < m_SwapChain->GetImageCount(); ++i)
        {
            HAL::FramebufferDesc framebufferDesc{};
            framebufferDesc.RenderPass        = m_RenderPass.GetRaw();
            framebufferDesc.RenderTargetViews = { m_RTVs[i] };
            framebufferDesc.Width             = m_Scissor.Width();
            framebufferDesc.Height            = m_Scissor.Height();
            auto& framebuffer                 = m_Framebuffers.Push(m_Device->CreateFramebuffer(framebufferDesc));

            auto& cmd = m_CommandBuffers.Push(m_Device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.GetRaw());
            cmd->BindDescriptorTables({ m_DescriptorTable.GetRaw() }, m_Pipeline.GetRaw());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_VertexBuffer.GetRaw());
            cmd->BindIndexBuffer(m_IndexBuffer.GetRaw());
            cmd->BeginRenderPass(m_RenderPass.GetRaw(), framebuffer.GetRaw(), { HAL::ClearValueDesc{ FE::Colors::Coral } });
            cmd->DrawIndexed(6, 1, 0, 0, 0);
            cmd->EndRenderPass();
            cmd->End();
        }
    }
};

FE_APP_MAIN()
{
    auto app = FE::MakeShared<TestApplication>();
    app->Initialize(FE::ApplicationDesc("Ferrum3D - Test"));
    return app->RunMainLoop();
}
