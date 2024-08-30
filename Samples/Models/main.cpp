#include <FeCore/Assets/Asset.h>
#include <FeCore/Framework/ApplicationModule.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/ShaderAssetStorage.h>
#include <Graphics/Module.h>
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

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* ExampleName = "Ferrum3D - Models";

class ExampleApplication final : public ApplicationModule
{
    ModuleDependency<HAL::OsmiumGPUModule> m_OsmiumGPUModule;
    ModuleDependency<OsmiumAssetsModule> m_OsmiumAssetsModule;

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

    Asset<ShaderAssetStorage> m_PixelShaderAsset;
    Asset<ShaderAssetStorage> m_VertexShaderAsset;

    Rc<HAL::Buffer> m_ConstantBuffer;

    Asset<MeshAssetStorage> m_MeshAsset;
    Asset<ImageAssetStorage> m_TextureAsset;
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

        m_GraphicsQueue = m_Device->GetCommandQueue(HAL::HardwareQueueKindFlags::kGraphics);
        m_TransferQueue = m_Device->GetCommandQueue(HAL::HardwareQueueKindFlags::kTransfer);

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

        m_MeshAsset = Asset<MeshAssetStorage>::LoadSynchronously(pAssetManager.Get(), "Models/cube");
        m_TextureAsset = Asset<ImageAssetStorage>::LoadSynchronously(pAssetManager.Get(), "Textures/image");
        m_TextureSampler = pServiceProvider->ResolveRequired<HAL::Sampler>();
        m_TextureSampler->Init(HAL::SamplerDesc{});

        {
            const float imageWidth = static_cast<float>(m_SwapChain->GetDesc().ImageWidth);
            const float imageHeight = static_cast<float>(m_SwapChain->GetDesc().ImageHeight);
            const float aspectRatio = imageWidth / imageHeight;

            auto constantData = Matrix4x4F::GetIdentity();
            constantData *= Matrix4x4F::CreateProjection(Math::Constants::PI * 0.5, aspectRatio, 0.1f, 10.0f);
            constantData *= Matrix4x4F::CreateRotationY(Math::Constants::PI);
            constantData *= Matrix4x4F::CreateRotationX(-0.5f);
            constantData *= Matrix4x4F::CreateTranslation(Vector3F(0.0f, 0.8f, -1.5f) * 2);
            constantData *= Matrix4x4F::CreateRotationY(Math::Constants::PI * -1.3f);

            m_ConstantBuffer = pServiceProvider->ResolveRequired<HAL::Buffer>();
            m_ConstantBuffer->Init("Constant buffer", HAL::BufferDesc(sizeof(constantData), HAL::BindFlags::ConstantBuffer));
            m_ConstantBuffer->AllocateMemory(HAL::MemoryType::kHostVisible);
            m_ConstantBuffer->UpdateData(constantData.RowMajorData());
        }

        m_VertexShaderAsset = Asset<ShaderAssetStorage>::LoadSynchronously(pAssetManager.Get(), "Shaders/VertexShader");
        m_PixelShaderAsset = Asset<ShaderAssetStorage>::LoadSynchronously(pAssetManager.Get(), "Shaders/PixelShader");

        HAL::RenderPassDesc renderPassDesc{};

        HAL::AttachmentDesc attachmentDesc{};
        attachmentDesc.Format = m_SwapChain->GetDesc().Format;
        attachmentDesc.InitialState = HAL::ResourceState::kUndefined;
        attachmentDesc.FinalState = HAL::ResourceState::kPresent;

        HAL::AttachmentDesc depthAttachmentDesc{};
        depthAttachmentDesc.Format = m_SwapChain->GetDSV()->GetDesc().Format;
        depthAttachmentDesc.StoreOp = HAL::AttachmentStoreOp::Store;
        depthAttachmentDesc.LoadOp = HAL::AttachmentLoadOp::Clear;
        depthAttachmentDesc.InitialState = HAL::ResourceState::kUndefined;
        depthAttachmentDesc.FinalState = HAL::ResourceState::kDepthWrite;

        std::array attachments{ attachmentDesc, depthAttachmentDesc };
        renderPassDesc.Attachments = attachments;

        HAL::SubpassDesc subpassDesc{};
        auto renderTargetAttachment = HAL::SubpassAttachment(HAL::ResourceState::kRenderTarget, 0);
        subpassDesc.RenderTargetAttachments = festd::span(&renderTargetAttachment, 1);
        subpassDesc.DepthStencilAttachment = HAL::SubpassAttachment(HAL::ResourceState::kDepthWrite, 1);
        renderPassDesc.Subpasses = festd::span(&subpassDesc, 1);
        HAL::SubpassDependency dependency{};
        renderPassDesc.SubpassDependencies = festd::span(&dependency, 1);

        m_RenderPass = pServiceProvider->ResolveRequired<HAL::RenderPass>();
        m_RenderPass->Init(renderPassDesc);

        HAL::ShaderModule* psModule = m_PixelShaderAsset->GetShaderModule();
        HAL::ShaderModule* vsModule = m_VertexShaderAsset->GetShaderModule();
        const HAL::ShaderReflection* psReflection = psModule->GetReflection();
        const HAL::ShaderReflection* vsReflection = vsModule->GetReflection();

        m_SRG = pServiceProvider->ResolveRequired<HAL::ShaderResourceGroup>();

        const std::array shadersReflection{ psReflection, vsReflection };
        m_SRG->Init({ shadersReflection });

        HAL::ShaderResourceGroupData srgData{};
        srgData.Set(psReflection->GetResourceBindingIndex("g_Sampler"), m_TextureSampler.Get());
        srgData.Set(psReflection->GetResourceBindingIndex("g_Texture"), m_TextureAsset->GetImageView());
        srgData.Set(vsReflection->GetResourceBindingIndex("Settings"), m_ConstantBuffer.Get());
        m_SRG->Update(srgData);

        HAL::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.InputLayout = HAL::InputLayoutBuilder(HAL::PrimitiveTopology::kTriangleList)
                                       .AddBuffer(HAL::InputStreamRate::kPerVertex)
                                       .AddAttribute(HAL::Format::kR32G32B32_SFLOAT, "POSITION")
                                       .AddAttribute(HAL::Format::kR32G32_SFLOAT, "TEXCOORD")
                                       .Build()
                                       .Build();

        pipelineDesc.RenderPass = m_RenderPass.Get();
        pipelineDesc.SubpassIndex = 0;

        const std::array colorBlendStates{ HAL::TargetColorBlending{} };
        pipelineDesc.ColorBlend = HAL::ColorBlendState(colorBlendStates);

        std::array shaders{ psModule, vsModule };
        pipelineDesc.Shaders = festd::span(shaders);

        std::array srgs{ m_SRG.Get() };
        pipelineDesc.ShaderResourceGroups = srgs;
        pipelineDesc.Viewport = m_Viewport;
        pipelineDesc.Scissor = m_Scissor;
        pipelineDesc.Rasterization = HAL::RasterizationState{};

        pipelineDesc.DepthStencil.DepthWriteEnabled = true;
        pipelineDesc.DepthStencil.DepthTestEnabled = true;
        pipelineDesc.DepthStencil.DepthCompareOp = HAL::CompareOp::kLess;

        pipelineDesc.Rasterization.CullMode = HAL::CullingModeFlags::kBack;

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
            cmd->Init({ HAL::HardwareQueueKindFlags::kGraphics, HAL::CommandListFlags::None });
            m_CommandLists.push_back(cmd);

            std::array clearValues{ HAL::ClearValueDesc::CreateColorValue(Colors::MediumAquamarine),
                                    HAL::ClearValueDesc::CreateDepthStencilValue() };

            cmd->Begin();
            cmd->BindGraphicsPipeline(m_Pipeline.Get());
            cmd->BindShaderResourceGroups(srgs, m_Pipeline.Get());
            cmd->SetViewport(m_Viewport);
            cmd->SetScissor(m_Scissor);
            cmd->BindVertexBuffer(0, m_MeshAsset->GetVertexBuffer(), 0);
            cmd->BindIndexBuffer(m_MeshAsset->GetIndexBuffer(), 0);
            cmd->BeginRenderPass(m_RenderPass.Get(), framebuffer.Get(), clearValues);
            cmd->DrawIndexed(m_MeshAsset->GetIndexCount(), 1, 0, 0, 0);
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
