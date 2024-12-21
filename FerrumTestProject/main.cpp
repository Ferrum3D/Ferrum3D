/*
 * Copyright 2022 Nikita Dubovikov
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

#include <FeCore/Logging/Trace.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Framework/ApplicationModule.h>
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

    explicit TestJob(int i, FE::JobPriority priority = FE::JobPriority::kNormal)
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

class TestApplication final : public FE::ApplicationModule
{
    FE::Rc<RHI::IInstance> m_Instance;
    FE::Rc<RHI::IAdapter> m_Adapter;
    FE::Rc<RHI::IDevice> m_Device;
    FE::Rc<RHI::TransientResourceHeap> m_ResourceHeap;

    FE::List<FE::Rc<RHI::IFence>> m_Fences;
    FE::List<FE::Rc<RHI::Framebuffer>> m_Framebuffers;
    FE::List<FE::Rc<RHI::ICommandBuffer>> m_CommandBuffers;
    FE::Rc<RHI::ICommandQueue> m_GraphicsQueue;
    FE::Rc<RHI::ICommandQueue> m_TransferQueue;

    FE::Rc<RHI::RenderPass> m_RenderPass;
    FE::Rc<RHI::Swapchain> m_SwapChain;
    FE::Rc<RHI::GraphicsPipeline> m_Pipeline;
    FE::List<RHI::IImageView*> m_RTVs;

    FE::Rc<RHI::IDescriptorHeap> m_DescriptorHeap;
    FE::Rc<RHI::IDescriptorTable> m_DescriptorTable;

    FE::Rc<RHI::ShaderModule> m_PixelShader;
    FE::Rc<RHI::ShaderModule> m_VertexShader;

    FE::Rc<RHI::IBuffer> m_ConstantBuffer;
    FE::Rc<RHI::IBuffer> m_IndexBuffer, m_VertexBuffer;

    FE::Rc<RHI::IWindow> m_Window;
    RHI::Viewport m_Viewport{};
    RHI::Scissor m_Scissor{};

    const FE::int32_t m_FrameBufferCount = 3;

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
        auto delta      = frameEventArgs.m_deltaTime;

        m_Fences[frameIndex]->WaitOnCPU();
        m_Window->PollEvents();
        auto imageIndex = m_SwapChain->GetCurrentImageIndex();
        m_Fences[m_SwapChain->GetCurrentFrameIndex()]->Reset();
        m_GraphicsQueue->SubmitBuffers(
            { m_CommandBuffers[imageIndex].Get() }, m_Fences[frameIndex].Get(), RHI::SubmitFlags::FrameBeginEnd);
        m_SwapChain->Present();

        if ((frameEventArgs.m_frameIndex + 1) % 1000 == 0)
        {
            FE_LOG_MESSAGE("Counter: {}", JobCounter);
            FE_LOG_MESSAGE("Delta: {}sec, FPS: {}", delta, 1.0f / delta);
        }
    }

    void GetFrameworkDependencies(FE::List<FE::Rc<FE::IModuleFactory>>& dependencies) override
    {
        dependencies.Push(RHI::OsmiumGPUModule::CreateFactory());
    }

public:
    FE_RTTI_Class(TestApplication, "04E7E607-FBE9-42C8-93E2-87E198AA1905");

    ~TestApplication() override
    {
        m_Device->WaitIdle();
    }

    void Initialize(const FE::ApplicationDesc& desc) override
    {
        ApplicationModule::Initialize(desc);
        auto module = FE::ServiceLocator<RHI::OsmiumGPUModule>::Get();
        module->Initialize(RHI::OsmiumGPUModuleDesc("Ferrum3D - Test", RHI::GraphicsAPI::Vulkan));

        FE::IO::StdoutStream stdoutStream;
        {
            FE::String b = "Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘\n";
            stdoutStream.WriteFromBuffer(b.Data(), b.Size());
        }

        FE::List<TestJob> testJobs;
        testJobs.Reserve(16 * 1024);
        for (int i = 0; i < 16 * 1024; ++i)
        {
            testJobs.Emplace(i).Schedule();
        }
        testJobs.Emplace(999999, FE::JobPriority::kHighest).Schedule();

        FE_LOG_MESSAGE("Counter: {}", JobCounter);

        FE_LOG_MESSAGE("Running {} version {}.{}.{}",
                       FE::StringSlice(FE::FerrumEngineName),
                       FE::FerrumVersion.Major,
                       FE::FerrumVersion.Minor,
                       FE::FerrumVersion.Patch);
        m_Instance      = module->CreateInstance();
        m_Adapter       = m_Instance->GetAdapters()[0];
        m_Device        = m_Adapter->CreateDevice();
        m_GraphicsQueue = m_Device->GetCommandQueue(RHI::CommandQueueClass::Graphics);
        m_TransferQueue = m_Device->GetCommandQueue(RHI::CommandQueueClass::Transfer);
        m_Window        = m_Device->CreateWindow(RHI::WindowDesc{ Desc.WindowWidth, Desc.WindowHeight, "Test project" });
        m_Viewport      = m_Window->CreateViewport();
        m_Scissor       = m_Window->CreateScissor();
        m_ResourceHeap  = m_Device->CreateTransientResourceHeap(RHI::TransientResourceHeapDesc{});
        m_ResourceHeap->Allocate();

        auto compiler = m_Device->CreateShaderCompiler();
        RHI::SwapchainDesc swapChainDesc{};
        swapChainDesc.ImageCount         = m_FrameBufferCount;
        swapChainDesc.ImageWidth         = m_Scissor.Width();
        swapChainDesc.ImageHeight        = m_Scissor.Height();
        swapChainDesc.NativeWindowHandle = m_Window->GetNativeHandle();
        swapChainDesc.Queue              = m_GraphicsQueue.Get();
        swapChainDesc.VerticalSync       = false;
        m_SwapChain                      = m_Device->CreateSwapChain(swapChainDesc);

        FE::Rc<RHI::IBuffer> indexBufferStaging, vertexBufferStaging;
        uint64_t vertexSize, indexSize;
        {
            // clang-format off
            FE::List<Vertex> vertexData = {
                { {-0.5f, -0.5f, 0.0f}, {1.0f, 0.1f, 0.1f} },
                { {+0.5f, +0.5f, 0.0f}, {0.1f, 1.0f, 0.1f} },
                { {+0.5f, -0.5f, 0.0f}, {0.1f, 0.1f, 1.0f} },
                { {-0.5f, +0.5f, 0.0f}, {1.0f, 1.0f, 0.1f} }
            };
            // clang-format on
            vertexSize          = vertexData.Size() * sizeof(Vertex);
            vertexBufferStaging = m_Device->CreateBuffer(RHI::BufferDesc(vertexSize, RHI::BindFlags::None));
            vertexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
            vertexBufferStaging->UpdateData(vertexData.Data());

            m_VertexBuffer = m_ResourceHeap->CreateBuffer(
                RHI::TransientBufferDesc(RHI::BufferDesc(vertexSize, RHI::BindFlags::VertexBuffer), 0));
        }
        {
            FE::List<uint32_t> indexData = { 0, 2, 3, 3, 2, 1 };
            indexSize                      = indexData.Size() * sizeof(uint32_t);
            indexBufferStaging             = m_Device->CreateBuffer(RHI::BufferDesc(indexSize, RHI::BindFlags::None));
            indexBufferStaging->AllocateMemory(RHI::MemoryType::kHostVisible);
            indexBufferStaging->UpdateData(indexData.Data());

            m_IndexBuffer = m_ResourceHeap->CreateBuffer(
                RHI::TransientBufferDesc(RHI::BufferDesc(indexSize, RHI::BindFlags::IndexBuffer), 1));
        }
        {
            FE::Color4F constantData = FE::Colors::White;
            constantData *= 0.7f;
            m_ConstantBuffer = m_Device->CreateBuffer(RHI::BufferDesc(sizeof(FE::Vector4F), RHI::BindFlags::ConstantBuffer));
            m_ConstantBuffer->AllocateMemory(RHI::MemoryType::kHostVisible);
            m_ConstantBuffer->UpdateData(constantData.Data());
        }

        {
            auto transferComplete = m_Device->CreateFence(RHI::FenceState::Reset);
            auto copyCmdBuffer    = m_Device->CreateCommandBuffer(RHI::CommandQueueClass::Transfer);
            copyCmdBuffer->Begin();
            copyCmdBuffer->CopyBuffers(vertexBufferStaging.Get(), m_VertexBuffer.Get(), RHI::BufferCopyRegion(vertexSize));
            copyCmdBuffer->CopyBuffers(indexBufferStaging.Get(), m_IndexBuffer.Get(), RHI::BufferCopyRegion(indexSize));
            copyCmdBuffer->End();
            m_TransferQueue->SubmitBuffers({ copyCmdBuffer.Get() }, transferComplete.Get(), RHI::SubmitFlags::None);
            transferComplete->WaitOnCPU();
        }

        RHI::ShaderCompilerArgs psArgs{};
        psArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
        psArgs.Stage      = RHI::ShaderStage::kPixel;
        psArgs.EntryPoint = "main";
        psArgs.FullPath   = "../../FerrumTestProject/Shaders/PixelShader.hlsl";
        auto psSource     = FE::IO::File::ReadAllText(psArgs.FullPath);
        psArgs.SourceCode = psSource;
        auto psByteCode   = compiler->CompileShader(psArgs);

        m_PixelShader = m_Device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kPixel, psByteCode));

        RHI::ShaderCompilerArgs vsArgs{};
        vsArgs.Version    = RHI::HLSLShaderVersion{ 6, 1 };
        vsArgs.Stage      = RHI::ShaderStage::kVertex;
        vsArgs.EntryPoint = "main";
        vsArgs.FullPath   = "../../FerrumTestProject/Shaders/VertexShader.hlsl";
        auto vsSource     = FE::IO::File::ReadAllText(vsArgs.FullPath);
        vsArgs.SourceCode = vsSource;
        auto vsByteCode   = compiler->CompileShader(vsArgs);

        m_VertexShader = m_Device->CreateShaderModule(RHI::ShaderModuleDesc(RHI::ShaderStage::kVertex, vsByteCode));

        RHI::RenderPassDesc renderPassDesc{};

        RHI::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format       = m_SwapChain->GetDesc().Format;
        attachmentDesc.StoreOp      = RHI::AttachmentStoreOp::Store;
        attachmentDesc.LoadOp       = RHI::AttachmentLoadOp::Clear;
        attachmentDesc.InitialState = RHI::ResourceState::kUndefined;
        attachmentDesc.FinalState   = RHI::ResourceState::kPresent;

        renderPassDesc.Attachments = FE::festd::span(&attachmentDesc, 1);

        RHI::SubpassDesc subpassDesc{};
        subpassDesc.RenderTargetAttachments = { RHI::SubpassAttachment(RHI::ResourceState::kRenderTarget, 0) };
        renderPassDesc.Subpasses            = FE::festd::span(&subpassDesc, 1);

        RHI::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = { dependency };

        m_RenderPass = m_Device->CreateRenderPass(renderPassDesc);

        RHI::DescriptorHeapDesc descriptorHeapDesc{};
        descriptorHeapDesc.MaxTables = 1;
        auto descriptorHeapSize      = RHI::DescriptorSize(1, RHI::ShaderResourceType::ConstantBuffer);
        descriptorHeapDesc.Sizes     = FE::festd::span(&descriptorHeapSize, 1);
        m_DescriptorHeap             = m_Device->CreateDescriptorHeap(descriptorHeapDesc);

        RHI::DescriptorDesc descriptorDesc(RHI::ShaderResourceType::ConstantBuffer, RHI::ShaderStageFlags::kPixel, 1);
        m_DescriptorTable = m_DescriptorHeap->AllocateDescriptorTable({ descriptorDesc });

        m_DescriptorTable->Update(RHI::DescriptorWriteBuffer(m_ConstantBuffer.Get()));

        RHI::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = RHI::InputLayoutBuilder(RHI::PrimitiveTopology::kTriangleList)
                                       .AddBuffer(RHI::InputStreamRate::kPerVertex)
                                       .AddAttribute(RHI::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(RHI::Format::R32G32B32_SFloat, "COLOR")
                                       .Build()
                                       .Build();

        pipelineDesc.RenderPass             = m_RenderPass.Get();
        pipelineDesc.SubpassIndex           = 0;
        pipelineDesc.ColorBlend             = RHI::ColorBlendState({ RHI::TargetColorBlending{} });
        auto shaders                        = FE::List{ m_PixelShader.Get(), m_VertexShader.Get() };
        pipelineDesc.Shaders                = shaders;
        auto descriptorTable                = m_DescriptorTable.Get();
        pipelineDesc.DescriptorTables       = FE::festd::span(&descriptorTable, 1);
        pipelineDesc.Viewport               = m_Viewport;
        pipelineDesc.Scissor                = m_Scissor;
        pipelineDesc.Rasterization          = RHI::RasterizationState{};
        pipelineDesc.Rasterization.CullMode = RHI::CullingModeFlags::kBack;

        m_Pipeline = m_Device->CreateGraphicsPipeline(pipelineDesc);

        for (FE::size_t i = 0; i < m_SwapChain->GetDesc().FrameCount; ++i)
        {
            m_Fences.Push(m_Device->CreateFence(RHI::FenceState::Signaled));
        }

        m_RTVs = m_SwapChain->GetRTVs();
        for (FE::size_t i = 0; i < m_SwapChain->GetImageCount(); ++i)
        {
            RHI::FramebufferDesc framebufferDesc{};
            framebufferDesc.RenderPass        = m_RenderPass.Get();
            framebufferDesc.RenderTargetViews = FE::festd::span(m_RTVs)(i, i + 1);
            framebufferDesc.Width             = m_Scissor.Width();
            framebufferDesc.Height            = m_Scissor.Height();
            auto& framebuffer                 = m_Framebuffers.Push(m_Device->CreateFramebuffer(framebufferDesc));

            auto& cmd = m_CommandBuffers.Push(m_Device->CreateCommandBuffer(RHI::CommandQueueClass::Graphics));
            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.Get());
            cmd->BindDescriptorTables({ m_DescriptorTable.Get() }, m_Pipeline.Get());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_VertexBuffer.Get(), 0);
            cmd->BindIndexBuffer(m_IndexBuffer.Get(), 0);
            cmd->BeginRenderPass(
                m_RenderPass.Get(), framebuffer.Get(), { RHI::ClearValueDesc::CreateColorValue(FE::Colors::Coral) });
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
