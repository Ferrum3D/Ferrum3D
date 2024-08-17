#include <FeCore/Assets/Asset.h>
#include <FeCore/Framework/ApplicationModule.h>
#include <FeCore/IO/FileHandle.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <OsAssets/OsmiumAssetsModule.h>
#include <OsGPU/OsmiumGPU.h>
#include <OsGPU/OsmiumGPUModule.h>

using namespace FE;

namespace HAL = FE::Osmium;

inline constexpr const char* ExampleName = "Ferrum3D - Models";

class ExampleApplication final : public ApplicationModule
{
    ModuleDependency<HAL::OsmiumGPUModule> m_OsmiumGPUModule;
    ModuleDependency<HAL::OsmiumAssetsModule> m_OsmiumAssetsModule;

    Rc<HAL::IInstance> m_Instance;
    Rc<HAL::IAdapter> m_Adapter;
    Rc<HAL::IDevice> m_Device;

    festd::vector<Rc<HAL::IFence>> m_Fences;
    festd::vector<Rc<HAL::IFramebuffer>> m_Framebuffers;
    festd::vector<Rc<HAL::ICommandBuffer>> m_CommandBuffers;
    Rc<HAL::ICommandQueue> m_GraphicsQueue;
    Rc<HAL::ICommandQueue> m_TransferQueue;

    Rc<HAL::IRenderPass> m_RenderPass;
    Rc<HAL::ISwapChain> m_SwapChain;
    Rc<HAL::IGraphicsPipeline> m_Pipeline;
    festd::vector<HAL::IImageView*> m_RTVs;

    Rc<HAL::IDescriptorHeap> m_DescriptorHeap;
    Rc<HAL::IDescriptorTable> m_DescriptorTable;

    Rc<HAL::IShaderModule> m_PixelShader;
    Rc<HAL::IShaderModule> m_VertexShader;

    Rc<HAL::IBuffer> m_ConstantBuffer;
    Rc<HAL::IBuffer> m_IndexBuffer, m_VertexBuffer;

    Rc<HAL::IImage> m_TextureImage;
    Rc<HAL::IImageView> m_TextureView;
    Rc<HAL::ISampler> m_TextureSampler;

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
        auto frameIndex = m_SwapChain->GetCurrentFrameIndex();

        m_Fences[frameIndex]->WaitOnCPU();
        m_Window->PollEvents();
        auto imageIndex = m_SwapChain->GetCurrentImageIndex();
        m_Fences[m_SwapChain->GetCurrentFrameIndex()]->Reset();
        m_GraphicsQueue->SubmitBuffers(
            { m_CommandBuffers[imageIndex].Get() }, m_Fences[frameIndex].Get(), HAL::SubmitFlags::FrameBeginEnd);
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

        m_Instance = Env::GetServiceProvider()->ResolveRequired<HAL::IInstance>();
        m_Adapter = m_Instance->GetAdapters()[0];
        m_Device = m_Adapter->CreateDevice();
        m_GraphicsQueue = m_Device->GetCommandQueue(HAL::CommandQueueClass::Graphics);
        m_TransferQueue = m_Device->GetCommandQueue(HAL::CommandQueueClass::Transfer);
        m_Window = m_Device->CreateWindow(HAL::WindowDesc{ 800, 600, ExampleName });
        m_Viewport = m_Window->CreateViewport();
        m_Scissor = m_Window->CreateScissor();

        HAL::SwapChainDesc swapChainDesc{};
        swapChainDesc.ImageCount = m_FrameBufferCount;
        swapChainDesc.ImageWidth = m_Scissor.Width();
        swapChainDesc.ImageHeight = m_Scissor.Height();
        swapChainDesc.NativeWindowHandle = m_Window->GetNativeHandle();
        swapChainDesc.Queue = m_GraphicsQueue.Get();
        swapChainDesc.VerticalSync = true;
        m_SwapChain = m_Device->CreateSwapChain(swapChainDesc);

        auto meshAsset = Assets::Asset<HAL::MeshAssetStorage>(Assets::AssetID("884FEDDD-141D-49A0-92B2-38B519403D0A"));
        meshAsset.LoadSync(pAssetManager.Get());
        Rc<HAL::IBuffer> indexBufferStaging, vertexBufferStaging;
        uint64_t vertexSize, indexSize;
        {
            vertexSize = meshAsset->VertexSize();
            vertexBufferStaging = m_Device->CreateBuffer(HAL::BufferDesc(vertexSize, HAL::BindFlags::None));
            vertexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            vertexBufferStaging->UpdateData(meshAsset->VertexData());

            m_VertexBuffer = m_Device->CreateBuffer(HAL::BufferDesc(vertexSize, HAL::BindFlags::VertexBuffer));
            m_VertexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }
        {
            indexSize = meshAsset->IndexSize();
            indexBufferStaging = m_Device->CreateBuffer(HAL::BufferDesc(indexSize, HAL::BindFlags::None));
            indexBufferStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            indexBufferStaging->UpdateData(meshAsset->IndexData());

            m_IndexBuffer = m_Device->CreateBuffer(HAL::BufferDesc(indexSize, HAL::BindFlags::IndexBuffer));
            m_IndexBuffer->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }

        Rc<HAL::IBuffer> textureStaging;
        {
            auto imageAsset = Assets::Asset<HAL::ImageAssetStorage>(Assets::AssetID("94FC6391-4656-4BE7-844D-8D87680A00F1"));
            imageAsset.LoadSync(pAssetManager.Get());

            textureStaging = m_Device->CreateBuffer(HAL::BufferDesc(imageAsset->Size(), HAL::BindFlags::None));
            textureStaging->AllocateMemory(HAL::MemoryType::HostVisible);
            textureStaging->UpdateData(imageAsset->Data());

            auto imageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::TransferWrite | HAL::ImageBindFlags::ShaderRead,
                                                   imageAsset->Width(),
                                                   imageAsset->Height(),
                                                   HAL::Format::R8G8B8A8_SRGB);

            m_TextureImage = m_Device->CreateImage(imageDesc);
            m_TextureImage->AllocateMemory(HAL::MemoryType::DeviceLocal);
        }

        {
            auto aspectRatio =
                static_cast<float>(m_SwapChain->GetDesc().ImageWidth) / static_cast<float>(m_SwapChain->GetDesc().ImageHeight);
            auto constantData = Matrix4x4F::GetIdentity();
            constantData *= Matrix4x4F::CreateProjection(Constants::PI * 0.5, aspectRatio, 0.1f, 10.0f);
            constantData *= Matrix4x4F::CreateRotationY(Constants::PI);
            constantData *= Matrix4x4F::CreateRotationX(-0.5f);
            constantData *= Matrix4x4F::CreateTranslation(Vector3F(0.0f, 0.8f, -1.5f) * 2);
            constantData *= Matrix4x4F::CreateRotationY(Constants::PI * -1.3f);
            m_ConstantBuffer = m_Device->CreateBuffer(HAL::BufferDesc(sizeof(constantData), HAL::BindFlags::ConstantBuffer));
            m_ConstantBuffer->AllocateMemory(HAL::MemoryType::HostVisible);
            m_ConstantBuffer->UpdateData(constantData.RowMajorData());
        }

        m_TextureView = m_TextureImage->CreateView(HAL::ImageAspectFlags::Color);
        m_TextureSampler = m_Device->CreateSampler(HAL::SamplerDesc{});

        {
            auto transferComplete = m_Device->CreateFence(HAL::FenceState::Reset);
            auto copyCmdBuffer = m_Device->CreateCommandBuffer(HAL::CommandQueueClass::Transfer);
            copyCmdBuffer->Begin();
            copyCmdBuffer->CopyBuffers(vertexBufferStaging.Get(), m_VertexBuffer.Get(), HAL::BufferCopyRegion(vertexSize));
            copyCmdBuffer->CopyBuffers(indexBufferStaging.Get(), m_IndexBuffer.Get(), HAL::BufferCopyRegion(indexSize));

            HAL::ImageBarrierDesc barrier{};
            barrier.Image = m_TextureImage.Get();
            barrier.SubresourceRange = m_TextureView->GetDesc().SubresourceRange;
            barrier.StateAfter = HAL::ResourceState::TransferWrite;
            copyCmdBuffer->ResourceTransitionBarriers({ barrier }, {});

            auto size = m_TextureImage->GetDesc().ImageSize;
            copyCmdBuffer->CopyBufferToImage(textureStaging.Get(), m_TextureImage.Get(), HAL::BufferImageCopyRegion(size));

            barrier.StateAfter = HAL::ResourceState::ShaderResource;
            copyCmdBuffer->ResourceTransitionBarriers({ barrier }, {});
            copyCmdBuffer->End();
            m_TransferQueue->SubmitBuffers({ copyCmdBuffer.Get() }, transferComplete.Get(), HAL::SubmitFlags::None);
            transferComplete->WaitOnCPU();
        }

        auto compiler = m_Device->CreateShaderCompiler();
        HAL::ShaderCompilerArgs shaderArgs{};
        shaderArgs.Version = HAL::HLSLShaderVersion{ 6, 1 };
        shaderArgs.EntryPoint = "main";

        auto vertexShaderAsset = Assets::Asset<HAL::ShaderAssetStorage>(Assets::AssetID("7C8B7FDD-3CE8-4286-A4C1-03D8A07CF338"));
        vertexShaderAsset.LoadSync(pAssetManager.Get());
        auto pixelShaderAsset = Assets::Asset<HAL::ShaderAssetStorage>(Assets::AssetID("90B76162-0BF0-45DF-A58B-13AFC834C551"));
        pixelShaderAsset.LoadSync(pAssetManager.Get());

        shaderArgs.Stage = HAL::ShaderStage::Pixel;
        shaderArgs.FullPath = "../../Samples/Models/Shaders/PixelShader.hlsl";
        shaderArgs.SourceCode = pixelShaderAsset->GetSourceCode();
        auto psByteCode = compiler->CompileShader(shaderArgs);

        shaderArgs.Stage = HAL::ShaderStage::Vertex;
        shaderArgs.FullPath = "../../Samples/Models/Shaders/VertexShader.hlsl";
        shaderArgs.SourceCode = vertexShaderAsset->GetSourceCode();
        auto vsByteCode = compiler->CompileShader(shaderArgs);
        compiler.Reset();

        m_PixelShader = m_Device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Pixel, psByteCode));
        m_VertexShader = m_Device->CreateShaderModule(HAL::ShaderModuleDesc(HAL::ShaderStage::Vertex, vsByteCode));

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

        eastl::vector attachments{ attachmentDesc, depthAttachmentDesc };
        renderPassDesc.Attachments = attachments;

        HAL::SubpassDesc subpassDesc{};
        auto renderTargetAttachment = HAL::SubpassAttachment(HAL::ResourceState::RenderTarget, 0);
        subpassDesc.RenderTargetAttachments = ArraySlice(&renderTargetAttachment, 1);
        subpassDesc.DepthStencilAttachment = HAL::SubpassAttachment(HAL::ResourceState::DepthWrite, 1);
        renderPassDesc.Subpasses = ArraySlice(&subpassDesc, 1);
        HAL::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = ArraySlice(&dependency, 1);
        m_RenderPass = m_Device->CreateRenderPass(renderPassDesc);

        HAL::DescriptorHeapDesc descriptorHeapDesc{};
        descriptorHeapDesc.MaxTables = 1;

        eastl::vector descriptorHeapSizes{ HAL::DescriptorSize(1, HAL::ShaderResourceType::Sampler),
                                           HAL::DescriptorSize(1, HAL::ShaderResourceType::TextureSRV),
                                           HAL::DescriptorSize(1, HAL::ShaderResourceType::ConstantBuffer) };
        descriptorHeapDesc.Sizes = descriptorHeapSizes;
        m_DescriptorHeap = m_Device->CreateDescriptorHeap(descriptorHeapDesc);

        HAL::DescriptorDesc psSamplerDescriptorDesc(HAL::ShaderResourceType::Sampler, HAL::ShaderStageFlags::Pixel, 1);
        HAL::DescriptorDesc psTextureDescriptorDesc(HAL::ShaderResourceType::TextureSRV, HAL::ShaderStageFlags::Pixel, 1);
        HAL::DescriptorDesc vsDescriptorDesc(HAL::ShaderResourceType::ConstantBuffer, HAL::ShaderStageFlags::Vertex, 1);
        m_DescriptorTable =
            m_DescriptorHeap->AllocateDescriptorTable({ psSamplerDescriptorDesc, psTextureDescriptorDesc, vsDescriptorDesc });

        HAL::DescriptorWriteSampler descriptorWriteSampler{ m_TextureSampler.Get() };
        m_DescriptorTable->Update(descriptorWriteSampler);
        HAL::DescriptorWriteImage descriptorWriteImage{ m_TextureView.Get(), 1 };
        m_DescriptorTable->Update(descriptorWriteImage);
        HAL::DescriptorWriteBuffer descriptorWrite{ m_ConstantBuffer.Get() };
        descriptorWrite.Binding = 2;
        m_DescriptorTable->Update(descriptorWrite);

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::TriangleList)
                                       .AddBuffer(HAL::InputStreamRate::PerVertex)
                                       .AddAttribute(HAL::Format::R32G32B32_SFloat, "POSITION")
                                       .AddAttribute(HAL::Format::R32G32_SFloat, "TEXCOORD")
                                       .Build()
                                       .Build();

        pipelineDesc.RenderPass = m_RenderPass.Get();
        pipelineDesc.SubpassIndex = 0;
        pipelineDesc.ColorBlend = HAL::ColorBlendState({ HAL::TargetColorBlending{} });
        auto shaders = eastl::vector{ m_PixelShader.Get(), m_VertexShader.Get() };
        pipelineDesc.Shaders = ArraySlice(shaders);
        auto descriptorTable = m_DescriptorTable.Get();
        pipelineDesc.DescriptorTables = ArraySlice(&descriptorTable, 1);
        pipelineDesc.Viewport = m_Viewport;
        pipelineDesc.Scissor = m_Scissor;
        pipelineDesc.Rasterization = HAL::RasterizationState{};

        pipelineDesc.DepthStencil.DepthWriteEnabled = true;
        pipelineDesc.DepthStencil.DepthTestEnabled = true;
        pipelineDesc.DepthStencil.DepthCompareOp = HAL::CompareOp::Less;

        pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::Back;

        m_Pipeline = m_Device->CreateGraphicsPipeline(pipelineDesc);

        for (uint32_t i = 0; i < m_SwapChain->GetDesc().FrameCount; ++i)
            m_Fences.push_back(m_Device->CreateFence(HAL::FenceState::Signaled));

        m_RTVs = m_SwapChain->GetRTVs();
        for (uint32_t i = 0; i < m_SwapChain->GetImageCount(); ++i)
        {
            HAL::FramebufferDesc framebufferDesc{};
            framebufferDesc.RenderPass = m_RenderPass.Get();
            auto views = eastl::vector{ m_RTVs[i], m_SwapChain->GetDSV() };
            framebufferDesc.RenderTargetViews = views;
            framebufferDesc.Width = m_Scissor.Width();
            framebufferDesc.Height = m_Scissor.Height();
            m_Framebuffers.push_back(m_Device->CreateFramebuffer(framebufferDesc));
            const auto& framebuffer = m_Framebuffers.back();

            m_CommandBuffers.push_back(m_Device->CreateCommandBuffer(HAL::CommandQueueClass::Graphics));
            auto& cmd = m_CommandBuffers.back();
            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.Get());
            cmd->BindDescriptorTables({ m_DescriptorTable.Get() }, m_Pipeline.Get());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_VertexBuffer.Get(), 0);
            cmd->BindIndexBuffer(m_IndexBuffer.Get(), 0);
            auto clearValues = eastl::vector{ HAL::ClearValueDesc::CreateColorValue(Colors::MediumAquamarine),
                                              HAL::ClearValueDesc::CreateDepthStencilValue() };
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
