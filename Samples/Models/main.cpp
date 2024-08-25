#include <FeCore/Assets/Asset.h>
#include <FeCore/Framework/ApplicationModule.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <HAL/Buffer.h>
#include <HAL/CommandList.h>
#include <HAL/CommandQueue.h>
#include <HAL/Device.h>
#include <HAL/DeviceFactory.h>
#include <HAL/Fence.h>
#include <HAL/Framebuffer.h>
#include <HAL/GraphicsPipeline.h>
#include <HAL/IWindow.h>
#include <HAL/Image.h>
#include <HAL/ImageView.h>
#include <HAL/InputLayoutBuilder.h>
#include <HAL/Module.h>
#include <HAL/RenderPass.h>
#include <HAL/Sampler.h>
#include <HAL/ShaderCompiler.h>
#include <HAL/ShaderModule.h>
#include <HAL/ShaderReflection.h>
#include <HAL/ShaderResourceGroup.h>
#include <HAL/Swapchain.h>
#include <OsAssets/OsmiumAssetsModule.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* ExampleName = "Ferrum3D - Models";

class ExampleApplication final : public ApplicationModule
{
    ModuleDependency<HAL::OsmiumGPUModule> m_OsmiumGPUModule;
    ModuleDependency<FE::Osmium::OsmiumAssetsModule> m_OsmiumAssetsModule;

    Rc<HAL::DeviceFactory> m_Factory;
    Rc<HAL::Device> m_Device;

    festd::vector<Rc<HAL::Fence>> m_Fences;
    festd::vector<Rc<HAL::Framebuffer>> m_Framebuffers;
    festd::vector<Rc<HAL::CommandList>> m_CommandLists;
    Rc<HAL::CommandQueue> m_GraphicsQueue;
    Rc<HAL::CommandQueue> m_TransferQueue;

    Rc<HAL::RenderPass> m_RenderPass;
    Rc<HAL::Swapchain> m_SwapChain;
    Rc<HAL::GraphicsPipeline> m_Pipeline;
    festd::vector<HAL::ImageView*> m_RTVs;

    Rc<HAL::ShaderResourceGroup> m_SRG;

    Rc<HAL::ShaderModule> m_PixelShader;
    Rc<HAL::ShaderModule> m_VertexShader;

    Rc<HAL::Buffer> m_ConstantBuffer;
    Rc<HAL::Buffer> m_IndexBuffer, m_VertexBuffer;

    Rc<HAL::Image> m_TextureImage;
    Rc<HAL::ImageView> m_TextureView;
    Rc<HAL::Sampler> m_TextureSampler;

    Rc<HAL::IWindow> m_Window;
    HAL::Viewport m_Viewport{};
    HAL::Scissor m_Scissor{};

    const int32_t m_FrameBufferCount = 3;

protected:
    void PollSystemEvents() override
    {
        m_Window->PollEvents();
    }

    bool CloseEventReceived() override
    {
        return m_Window->CloseRequested();
    }

    void Tick(const FrameEventArgs& /* frameEventArgs */) override
    {
        const uint32_t frameIndex = m_SwapChain->GetCurrentFrameIndex();

        m_Fences[frameIndex]->WaitOnCPU();
        m_Window->PollEvents();

        const uint32_t imageIndex = m_SwapChain->GetCurrentImageIndex();
        m_Fences[m_SwapChain->GetCurrentFrameIndex()]->Reset();

        std::array commandLists{ m_CommandLists[imageIndex].Get() };
        m_GraphicsQueue->SubmitBuffers(commandLists, m_Fences[frameIndex].Get(), HAL::SubmitFlags::FrameBeginEnd);
        m_SwapChain->Present();
    }

public:
    FE_RTTI_Class(ExampleApplication, "78304A61-C92E-447F-9834-4D547B1D950F");

    ~ExampleApplication() override
    {
        m_Device->WaitIdle();
    }

    void Initialize() override
    {
        m_AssetDirectory = "../../../Samples/Models";
        ApplicationModule::Initialize();

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
        Rc pAssetManager = pServiceProvider->ResolveRequired<Assets::IAssetManager>();

        m_Factory = pServiceProvider->ResolveRequired<HAL::DeviceFactory>();
        m_Device = pServiceProvider->ResolveRequired<HAL::Device>();

        const HAL::AdapterInfo adapterInfo = m_Factory->EnumerateAdapters()[0];
        m_Factory->CreateDevice(adapterInfo.Name);

        m_GraphicsQueue = m_Device->GetCommandQueue(HAL::HardwareQueueKindFlags::Graphics);
        m_TransferQueue = m_Device->GetCommandQueue(HAL::HardwareQueueKindFlags::Transfer);

        m_Window = pServiceProvider->ResolveRequired<HAL::IWindow>();
        m_Window->Init(HAL::WindowDesc{ 800, 600, ExampleName });
        m_Viewport = m_Window->CreateViewport();
        m_Scissor = m_Window->CreateScissor();

        HAL::SwapchainDesc swapchainDesc{};
        swapchainDesc.ImageCount = m_FrameBufferCount;
        swapchainDesc.ImageWidth = m_Scissor.Width();
        swapchainDesc.ImageHeight = m_Scissor.Height();
        swapchainDesc.NativeWindowHandle = m_Window->GetNativeHandle();
        swapchainDesc.Queue = m_GraphicsQueue.Get();
        swapchainDesc.VerticalSync = true;
        m_SwapChain = pServiceProvider->ResolveRequired<HAL::Swapchain>();
        m_SwapChain->Init(swapchainDesc);

        auto meshAsset = Assets::Asset<Osmium::MeshAssetStorage>(Assets::AssetID("884FEDDD-141D-49A0-92B2-38B519403D0A"));
        meshAsset.LoadSync(pAssetManager.Get());

        Rc<HAL::Buffer> indexBufferStaging, vertexBufferStaging;
        uint64_t vertexSize, indexSize;
        {
            vertexSize = meshAsset->VertexSize();
            vertexBufferStaging = pServiceProvider->ResolveRequired<HAL::Buffer>();
            vertexBufferStaging->Init("Staging vertex", HAL::BufferDesc(vertexSize, HAL::BindFlags::None));
            vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            vertexBufferStaging->UpdateData(meshAsset->VertexData());

            m_VertexBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();
            m_VertexBuffer->Init("Vertex", HAL::BufferDesc(vertexSize, HAL::BindFlags::VertexBuffer));
            m_VertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            indexSize = meshAsset->IndexSize();
            indexBufferStaging = pServiceProvider->ResolveRequired<HAL::Buffer>();
            indexBufferStaging->Init("Staging index", HAL::BufferDesc(indexSize, HAL::BindFlags::None));
            indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            indexBufferStaging->UpdateData(meshAsset->IndexData());

            m_IndexBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();
            m_IndexBuffer->Init("Index", HAL::BufferDesc(indexSize, HAL::BindFlags::IndexBuffer));
            m_IndexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }

        Rc<HAL::Buffer> textureStaging;
        {
            auto imageAsset = Assets::Asset<Osmium::ImageAssetStorage>(Assets::AssetID("94FC6391-4656-4BE7-844D-8D87680A00F1"));
            imageAsset.LoadSync(pAssetManager.Get());

            textureStaging = pServiceProvider->ResolveRequired<HAL::Buffer>();
            textureStaging->Init("Staging texture", HAL::BufferDesc(imageAsset->Size(), HAL::BindFlags::None));
            textureStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            textureStaging->UpdateData(imageAsset->Data());

            auto imageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::TransferWrite | HAL::ImageBindFlags::ShaderRead,
                                                   imageAsset->Width(),
                                                   imageAsset->Height(),
                                                   HAL::Format::R8G8B8A8_SRGB);

            m_TextureImage = pServiceProvider->ResolveRequired<HAL::Image>();
            m_TextureImage->Init("Texture", imageDesc);
            m_TextureImage->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }

        {
            const float imageWidth = static_cast<float>(m_SwapChain->GetDesc().ImageWidth);
            const float imageHeight = static_cast<float>(m_SwapChain->GetDesc().ImageHeight);
            const float aspectRatio = imageWidth / imageHeight;

            auto constantData = Matrix4x4F::GetIdentity();
            constantData *= Matrix4x4F::CreateProjection(Constants::PI * 0.5, aspectRatio, 0.1f, 10.0f);
            constantData *= Matrix4x4F::CreateRotationY(Constants::PI);
            constantData *= Matrix4x4F::CreateRotationX(-0.5f);
            constantData *= Matrix4x4F::CreateTranslation(Vector3F(0.0f, 0.8f, -1.5f) * 2);
            constantData *= Matrix4x4F::CreateRotationY(Constants::PI * -1.3f);

            m_ConstantBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();
            m_ConstantBuffer->Init("Constant buffer", HAL::BufferDesc(sizeof(constantData), HAL::BindFlags::ConstantBuffer));
            m_ConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
            m_ConstantBuffer->UpdateData(constantData.RowMajorData());
        }

        m_TextureView = pServiceProvider->ResolveRequired<HAL::ImageView>();
        m_TextureView->Init(HAL::ImageViewDesc::ForImage(m_TextureImage.Get(), HAL::ImageAspectFlags::Color));
        m_TextureSampler = pServiceProvider->ResolveRequired<HAL::Sampler>();
        m_TextureSampler->Init(HAL::SamplerDesc{});

        {
            Rc transferComplete = pServiceProvider->ResolveRequired<HAL::Fence>();
            transferComplete->Init(HAL::FenceState::Reset);

            Rc copyCmdList = pServiceProvider->ResolveRequired<HAL::CommandList>();
            copyCmdList->Init({ HAL::HardwareQueueKindFlags::Transfer, HAL::CommandListFlags::OneTimeSubmit });
            copyCmdList->Begin();
            copyCmdList->CopyBuffers(vertexBufferStaging.Get(), m_VertexBuffer.Get(), HAL::BufferCopyRegion(vertexSize));
            copyCmdList->CopyBuffers(indexBufferStaging.Get(), m_IndexBuffer.Get(), HAL::BufferCopyRegion(indexSize));

            HAL::ImageBarrierDesc barrier{};
            barrier.Image = m_TextureImage.Get();
            barrier.SubresourceRange = m_TextureView->GetDesc().SubresourceRange;
            barrier.StateAfter = HAL::ResourceState::TransferWrite;
            copyCmdList->ResourceTransitionBarriers(std::array{ barrier }, {});

            const HAL::Size size = m_TextureImage->GetDesc().ImageSize;
            copyCmdList->CopyBufferToImage(textureStaging.Get(), m_TextureImage.Get(), HAL::BufferImageCopyRegion(size));

            barrier.StateAfter = HAL::ResourceState::ShaderResource;
            copyCmdList->ResourceTransitionBarriers(std::array{ barrier }, {});
            copyCmdList->End();

            std::array commandLists{ copyCmdList.Get() };
            m_TransferQueue->SubmitBuffers(commandLists, transferComplete.Get(), HAL::SubmitFlags::None);
            transferComplete->WaitOnCPU();
        }

        Rc compiler = pServiceProvider->ResolveRequired<HAL::ShaderCompiler>();
        HAL::ShaderCompilerArgs shaderArgs{};
        shaderArgs.Version = HAL::HLSLShaderVersion{ 6, 1 };
        shaderArgs.EntryPoint = "main";

        auto vertexShaderAsset =
            Assets::Asset<Osmium::ShaderAssetStorage>(Assets::AssetID("7C8B7FDD-3CE8-4286-A4C1-03D8A07CF338"));
        vertexShaderAsset.LoadSync(pAssetManager.Get());
        auto pixelShaderAsset =
            Assets::Asset<Osmium::ShaderAssetStorage>(Assets::AssetID("90B76162-0BF0-45DF-A58B-13AFC834C551"));
        pixelShaderAsset.LoadSync(pAssetManager.Get());

        shaderArgs.Stage = HAL::ShaderStage::Pixel;
        shaderArgs.FullPath = "../../Samples/Models/Shaders/PixelShader.hlsl";
        shaderArgs.SourceCode = pixelShaderAsset->GetSourceCode();
        const auto psByteCode = compiler->CompileShader(shaderArgs);

        shaderArgs.Stage = HAL::ShaderStage::Vertex;
        shaderArgs.FullPath = "../../Samples/Models/Shaders/VertexShader.hlsl";
        shaderArgs.SourceCode = vertexShaderAsset->GetSourceCode();
        const auto vsByteCode = compiler->CompileShader(shaderArgs);
        compiler.Reset();

        m_PixelShader = pServiceProvider->ResolveRequired<HAL::ShaderModule>();
        m_PixelShader->Init(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));
        m_VertexShader = pServiceProvider->ResolveRequired<HAL::ShaderModule>();
        m_VertexShader->Init(HAL::ShaderModuleDesc(HAL::ShaderStage::Vertex, vsByteCode));

        HAL::RenderPassDesc renderPassDesc{};

        HAL::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format = m_SwapChain->GetDesc().Format;
        attachmentDesc.InitialState = HAL::ResourceState::Undefined;
        attachmentDesc.FinalState = HAL::ResourceState::Present;

        HAL::AttachmentDesc depthAttachmentDesc{};
        depthAttachmentDesc.Format = m_SwapChain->GetDSV()->GetDesc().Format;
        depthAttachmentDesc.StoreOp = HAL::AttachmentStoreOp::Store;
        depthAttachmentDesc.LoadOp = HAL::AttachmentLoadOp::Clear;
        depthAttachmentDesc.InitialState = HAL::ResourceState::Undefined;
        depthAttachmentDesc.FinalState = HAL::ResourceState::DepthWrite;

        std::array attachments{ attachmentDesc, depthAttachmentDesc };
        renderPassDesc.Attachments = attachments;

        HAL::SubpassDesc subpassDesc{};
        auto renderTargetAttachment = HAL::SubpassAttachment(HAL::ResourceState::RenderTarget, 0);
        subpassDesc.RenderTargetAttachments = festd::span(&renderTargetAttachment, 1);
        subpassDesc.DepthStencilAttachment = HAL::SubpassAttachment(HAL::ResourceState::DepthWrite, 1);
        renderPassDesc.Subpasses = festd::span(&subpassDesc, 1);
        HAL::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = festd::span(&dependency, 1);

        m_RenderPass = pServiceProvider->ResolveRequired<HAL::RenderPass>();
        m_RenderPass->Init(renderPassDesc);

        const HAL::ShaderReflection* psReflection = m_PixelShader->GetReflection();
        const HAL::ShaderReflection* vsReflection = m_VertexShader->GetReflection();

        m_SRG = pServiceProvider->ResolveRequired<HAL::ShaderResourceGroup>();

        const std::array shadersReflection{ psReflection, vsReflection };
        m_SRG->Init({ shadersReflection });

        HAL::ShaderResourceGroupData srgData{};
        srgData.Set(psReflection->GetResourceBindingIndex("g_Sampler"), m_TextureSampler.Get());
        srgData.Set(psReflection->GetResourceBindingIndex("g_Texture"), m_TextureView.Get());
        srgData.Set(vsReflection->GetResourceBindingIndex("Settings"), m_ConstantBuffer.Get());
        m_SRG->Update(srgData);

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
                                       .AddBuffer(HAL::InputStreamRate::PerVertex)
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(HAL::Format::R32G32_SFloat, "TEXCOORD")
                                       .Build()
                                       .Build();

        pipelineDesc.RenderPass = m_RenderPass.Get();
        pipelineDesc.SubpassIndex = 0;

        const std::array colorBlendStates{ HAL::TargetColorBlending{} };
        pipelineDesc.ColorBlend = HAL::ColorBlendState(colorBlendStates);

        std::array shaders{ m_PixelShader.Get(), m_VertexShader.Get() };
        pipelineDesc.Shaders = festd::span(shaders);

        std::array srgs{ m_SRG.Get() };
        pipelineDesc.ShaderResourceGroups = srgs;
        pipelineDesc.Viewport = m_Viewport;
        pipelineDesc.Scissor = m_Scissor;
        pipelineDesc.Rasterization = HAL::RasterizationState{};

        pipelineDesc.DepthStencil.DepthWriteEnabled = true;
        pipelineDesc.DepthStencil.DepthTestEnabled = true;
        pipelineDesc.DepthStencil.DepthCompareOp = HAL::CompareOp::Less;

        pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

        m_Pipeline = pServiceProvider->ResolveRequired<HAL::GraphicsPipeline>();
        m_Pipeline->Init(pipelineDesc);

        for (uint32_t i = 0; i < m_SwapChain->GetDesc().FrameCount; ++i)
        {
            Rc fence = pServiceProvider->ResolveRequired<HAL::Fence>();
            fence->Init(HAL::FenceState::Signaled);
            m_Fences.push_back(fence);
        }

        const festd::span rtvSpan = m_SwapChain->GetRTVs();
        m_RTVs.resize(rtvSpan.size());
        Memory::Copy(rtvSpan, festd::span(m_RTVs));

        for (uint32_t i = 0; i < m_SwapChain->GetImageCount(); ++i)
        {
            HAL::FramebufferDesc framebufferDesc{};
            framebufferDesc.RenderPass = m_RenderPass.Get();
            auto views = eastl::vector{ m_RTVs[i], m_SwapChain->GetDSV() };
            framebufferDesc.RenderTargetViews = views;
            framebufferDesc.Width = m_Scissor.Width();
            framebufferDesc.Height = m_Scissor.Height();

            Rc framebuffer = pServiceProvider->ResolveRequired<HAL::Framebuffer>();
            framebuffer->Init(framebufferDesc);
            m_Framebuffers.push_back(framebuffer);

            Rc cmd = pServiceProvider->ResolveRequired<HAL::CommandList>();
            cmd->Init({ HAL::HardwareQueueKindFlags::Graphics, HAL::CommandListFlags::None });
            m_CommandLists.push_back(cmd);

            std::array clearValues{ HAL::ClearValueDesc::CreateColorValue(Colors::MediumAquamarine),
                                    HAL::ClearValueDesc::CreateDepthStencilValue() };

            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.Get());
            cmd->BindShaderResourceGroups(srgs, m_Pipeline.Get());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_VertexBuffer.Get(), 0);
            cmd->BindIndexBuffer(m_IndexBuffer.Get(), 0);
            cmd->BeginRenderPass(m_RenderPass.Get(), framebuffer.Get(), clearValues);
            cmd->DrawIndexed(meshAsset->IndexSize() / sizeof(uint32_t), 1, 0, 0, 0);
            cmd->EndRenderPass();
            cmd->End();
        }
    }
};

int main(int argc, char** argv)
{
    return ApplicationModule::Run<ExampleApplication>(argc, argv, [](ExampleApplication* app) {
        app->Initialize();
    });
}
