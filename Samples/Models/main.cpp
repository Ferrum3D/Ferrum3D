#include <FeCore/Assets/Asset.h>
#include <FeCore/Framework/ApplicationModule.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/ShaderAssetStorage.h>
#include <Graphics/Module.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/CommandList.h>
#include <Graphics/RHI/CommandQueue.h>
#include <Graphics/RHI/Device.h>
#include <Graphics/RHI/DeviceFactory.h>
#include <Graphics/RHI/Fence.h>
#include <Graphics/RHI/Framebuffer.h>
#include <Graphics/RHI/GraphicsPipeline.h>
#include <Graphics/RHI/IWindow.h>
#include <Graphics/RHI/ImageView.h>
#include <Graphics/RHI/InputLayoutBuilder.h>
#include <Graphics/RHI/Module.h>
#include <Graphics/RHI/RenderPass.h>
#include <Graphics/RHI/ResourcePool.h>
#include <Graphics/RHI/Sampler.h>
#include <Graphics/RHI/ShaderCompiler.h>
#include <Graphics/RHI/ShaderModule.h>
#include <Graphics/RHI/ShaderReflection.h>
#include <Graphics/RHI/ShaderResourceGroup.h>
#include <Graphics/RHI/Swapchain.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - Models";

struct ExampleApplication final : public ApplicationModule
{
    FE_RTTI_Class(ExampleApplication, "78304A61-C92E-447F-9834-4D547B1D950F");

    ExampleApplication(int32_t argc, const char** argv)
        : ApplicationModule(argc, argv)
    {
    }

    ~ExampleApplication() override
    {
        m_device->WaitIdle();
    }

    void Initialize() override
    {
        ZoneScoped;

        ApplicationModule::Initialize();

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();
        const Rc assetManager = pServiceProvider->ResolveRequired<Assets::IAssetManager>();

        m_factory = pServiceProvider->ResolveRequired<RHI::DeviceFactory>();
        m_device = pServiceProvider->ResolveRequired<RHI::Device>();

        const RHI::AdapterInfo adapterInfo = m_factory->EnumerateAdapters()[0];
        m_factory->CreateDevice(adapterInfo.m_name);

        m_graphicsQueue = m_device->GetCommandQueue(RHI::HardwareQueueKindFlags::kGraphics);
        m_transferQueue = m_device->GetCommandQueue(RHI::HardwareQueueKindFlags::kTransfer);

        const Rc resourcePool = pServiceProvider->ResolveRequired<RHI::ResourcePool>();

        m_window = pServiceProvider->ResolveRequired<RHI::IWindow>();
        m_window->Init(RHI::WindowDesc{ 800, 600, kExampleName });
        m_viewport = m_window->CreateViewport();
        m_scissor = m_window->CreateScissor();

        RHI::SwapchainDesc swapchainDesc{};
        swapchainDesc.m_frameCount = 2;
        swapchainDesc.m_imageWidth = m_scissor.Width();
        swapchainDesc.m_imageHeight = m_scissor.Height();
        swapchainDesc.m_nativeWindowHandle = m_window->GetNativeHandle();
        swapchainDesc.m_queue = m_graphicsQueue.Get();
        swapchainDesc.m_verticalSync = true;
        m_swapChain = pServiceProvider->ResolveRequired<RHI::Swapchain>();
        m_swapChain->Init(swapchainDesc);

        m_meshAsset = Asset<MeshAssetStorage>::LoadSynchronously(assetManager.Get(), "Models/cube");
        m_textureAsset = Asset<ImageAssetStorage>::LoadSynchronously(assetManager.Get(), "Textures/image");
        m_textureSampler = pServiceProvider->ResolveRequired<RHI::Sampler>();
        m_textureSampler->Init(RHI::SamplerDesc{});

        {
            const float imageWidth = static_cast<float>(m_swapChain->GetDesc().m_imageWidth);
            const float imageHeight = static_cast<float>(m_swapChain->GetDesc().m_imageHeight);
            const float aspectRatio = imageWidth / imageHeight;

            auto constantData = Matrix4x4F::Identity();
            constantData = constantData * Matrix4x4F::Projection(Math::Constants::PI * 0.5, aspectRatio, 0.1f, 10.0f);
            constantData = constantData * Matrix4x4F::RotationY(Math::Constants::PI);
            constantData = constantData * Matrix4x4F::RotationX(-0.5f);
            constantData = constantData * Matrix4x4F::Translation(Vector3F(0.0f, 0.8f, -1.5f) * 2);
            constantData = constantData * Matrix4x4F::RotationY(Math::Constants::PI * -1.3f);

            const auto constantBufferDesc =
                RHI::BufferDesc(sizeof(constantData), RHI::BindFlags::kConstantBuffer, RHI::ResourceUsage::kHostWriteThrough);
            m_constantBuffer = resourcePool->CreateBuffer("Constant Buffer", constantBufferDesc).value();
            m_constantBuffer->UpdateData(constantData.RowMajorData());
        }

        m_vertexShaderAsset = Asset<ShaderAssetStorage>::LoadSynchronously(assetManager.Get(), "Shaders/Shader.vs");
        m_pixelShaderAsset = Asset<ShaderAssetStorage>::LoadSynchronously(assetManager.Get(), "Shaders/Shader.ps");

        RHI::RenderPassDesc renderPassDesc{};

        RHI::AttachmentDesc attachmentDesc{};
        attachmentDesc.m_format = m_swapChain->GetDesc().m_format;
        attachmentDesc.m_initialState = RHI::ResourceState::kUndefined;
        attachmentDesc.m_finalState = RHI::ResourceState::kPresent;

        RHI::AttachmentDesc depthAttachmentDesc{};
        depthAttachmentDesc.m_format = m_swapChain->GetDSV()->GetDesc().m_format;
        depthAttachmentDesc.m_storeOp = RHI::AttachmentStoreOp::kStore;
        depthAttachmentDesc.m_loadOp = RHI::AttachmentLoadOp::kClear;
        depthAttachmentDesc.m_initialState = RHI::ResourceState::kUndefined;
        depthAttachmentDesc.m_finalState = RHI::ResourceState::kDepthWrite;

        const std::array attachments{ attachmentDesc, depthAttachmentDesc };
        renderPassDesc.m_attachments = attachments;

        RHI::SubpassDesc subpassDesc{};
        const RHI::SubpassAttachment renderTargetAttachment{ RHI::ResourceState::kRenderTarget, 0 };
        subpassDesc.m_renderTargetAttachments = festd::span{ &renderTargetAttachment, 1 };
        subpassDesc.m_depthStencilAttachment = RHI::SubpassAttachment{ RHI::ResourceState::kDepthWrite, 1 };
        renderPassDesc.m_subpasses = festd::span(&subpassDesc, 1);
        RHI::SubpassDependency dependency{};
        renderPassDesc.m_subpassDependencies = festd::span(&dependency, 1);

        m_renderPass = pServiceProvider->ResolveRequired<RHI::RenderPass>();
        m_renderPass->Init(renderPassDesc);

        RHI::ShaderModule* psModule = m_pixelShaderAsset->GetShaderModule();
        RHI::ShaderModule* vsModule = m_vertexShaderAsset->GetShaderModule();
        const RHI::ShaderReflection* psReflection = psModule->GetReflection();
        const RHI::ShaderReflection* vsReflection = vsModule->GetReflection();

        m_srg = pServiceProvider->ResolveRequired<RHI::ShaderResourceGroup>();

        const std::array shadersReflection{ psReflection, vsReflection };
        m_srg->Init({ shadersReflection });

        RHI::ShaderResourceGroupData srgData{};
        srgData.Set(psReflection->GetResourceBindingIndex("g_Sampler"), m_textureSampler.Get());
        srgData.Set(psReflection->GetResourceBindingIndex("g_Texture"), m_textureAsset->GetImageView());
        srgData.Set(vsReflection->GetResourceBindingIndex("Settings"), m_constantBuffer.Get());
        m_srg->Update(srgData);

        RHI::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.m_inputLayout = RHI::InputLayoutBuilder(RHI::PrimitiveTopology::kTriangleList)
                                         .AddBuffer(RHI::InputStreamRate::kPerVertex)
                                         .AddAttribute(RHI::Format::kR32G32B32_SFLOAT, "POSITION")
                                         .AddAttribute(RHI::Format::kR32G32_SFLOAT, "TEXCOORD")
                                         .Build()
                                         .Build();

        pipelineDesc.m_renderPass = m_renderPass.Get();
        pipelineDesc.m_subpassIndex = 0;

        const std::array colorBlendStates{ RHI::TargetColorBlending{} };
        pipelineDesc.m_colorBlend = RHI::ColorBlendState(colorBlendStates);

        const std::array shaders{ psModule, vsModule };
        pipelineDesc.m_shaders = festd::span(shaders);

        const std::array srgs{ m_srg.Get() };
        pipelineDesc.m_shaderResourceGroups = srgs;
        pipelineDesc.m_viewport = m_viewport;
        pipelineDesc.m_scissor = m_scissor;
        pipelineDesc.m_rasterization = RHI::RasterizationState{};

        pipelineDesc.m_depthStencil.m_depthWriteEnabled = true;
        pipelineDesc.m_depthStencil.m_depthTestEnabled = true;
        pipelineDesc.m_depthStencil.m_depthCompareOp = RHI::CompareOp::kLess;

        pipelineDesc.m_rasterization.m_cullMode = RHI::CullingModeFlags::kBack;

        m_pipeline = pServiceProvider->ResolveRequired<RHI::GraphicsPipeline>();
        m_pipeline->Init(pipelineDesc);

        m_fence = pServiceProvider->ResolveRequired<RHI::Fence>();
        m_fence->Init();
        m_fenceValue = 0;

        for (uint32_t i = 0; i < m_swapChain->GetDesc().m_frameCount; ++i)
            m_fenceValues.push_back(0);

        const festd::span rtvSpan = m_swapChain->GetRTVs();
        m_rTVs.resize(rtvSpan.size());
        Memory::Copy(rtvSpan, festd::span(m_rTVs));

        for (uint32_t i = 0; i < m_swapChain->GetImageCount(); ++i)
        {
            RHI::FramebufferDesc framebufferDesc{};
            framebufferDesc.m_renderPass = m_renderPass.Get();
            const auto views = eastl::vector{ m_rTVs[i], m_swapChain->GetDSV() };
            framebufferDesc.m_renderTargetViews = views;
            framebufferDesc.m_width = m_scissor.Width();
            framebufferDesc.m_height = m_scissor.Height();

            const Rc framebuffer = pServiceProvider->ResolveRequired<RHI::Framebuffer>();
            framebuffer->Init(framebufferDesc);
            m_framebuffers.push_back(framebuffer);

            const Rc cmd = pServiceProvider->ResolveRequired<RHI::CommandList>();
            cmd->Init({ RHI::HardwareQueueKindFlags::kGraphics, RHI::CommandListFlags::kNone });
            m_commandLists.push_back(cmd);

            const std::array clearValues{ RHI::ClearValueDesc::CreateColorValue(Colors::kMediumAquamarine),
                                          RHI::ClearValueDesc::CreateDepthStencilValue() };

            cmd->Begin();
            cmd->BindGraphicsPipeline(m_pipeline.Get());
            cmd->BindShaderResourceGroups(srgs, m_pipeline.Get());
            cmd->SetViewport(m_viewport);
            cmd->SetScissor(m_scissor);
            cmd->BindVertexBuffer(0, m_meshAsset->GetVertexBuffer(), 0);
            cmd->BindIndexBuffer(m_meshAsset->GetIndexBuffer(), 0);
            cmd->BeginRenderPass(m_renderPass.Get(), framebuffer.Get(), clearValues);
            cmd->DrawIndexed(m_meshAsset->GetIndexCount(), 1, 0, 0, 0);
            cmd->EndRenderPass();
            cmd->End();
        }
    }

private:
    void PollSystemEvents() override
    {
        m_window->PollEvents();
    }

    bool CloseEventReceived() override
    {
        return m_window->CloseRequested();
    }

    void Tick(const FrameEventArgs& frameEventArgs) override
    {
        m_window->PollEvents();
        if (m_window->CloseRequested())
            return;

        const uint32_t frameIndex = frameEventArgs.m_frameIndex % m_fenceValues.size();
        m_fence->Wait(m_fenceValues[frameIndex]);
        m_swapChain->BeginFrame({ m_fence, ++m_fenceValue });

        const std::array commandLists{ m_commandLists[frameIndex].Get() };

        m_fenceValues[frameIndex] = ++m_fenceValue;
        m_graphicsQueue->Execute(commandLists);
        m_graphicsQueue->SignalFence({ m_fence, m_fenceValue });
        m_swapChain->Present({ m_fence, m_fenceValue });
    }

    ModuleDependency<RHI::GraphicsRHIModule> m_rhiModule;
    ModuleDependency<GraphicsModule> m_graphicsModule;

    Rc<RHI::DeviceFactory> m_factory;
    Rc<RHI::Device> m_device;

    Rc<RHI::Fence> m_fence;
    uint64_t m_fenceValue = 0;
    festd::vector<uint64_t> m_fenceValues;

    festd::vector<Rc<RHI::Framebuffer>> m_framebuffers;
    festd::vector<Rc<RHI::CommandList>> m_commandLists;
    Rc<RHI::CommandQueue> m_graphicsQueue;
    Rc<RHI::CommandQueue> m_transferQueue;

    Rc<RHI::RenderPass> m_renderPass;
    Rc<RHI::Swapchain> m_swapChain;
    Rc<RHI::GraphicsPipeline> m_pipeline;
    festd::vector<RHI::ImageView*> m_rTVs;

    Rc<RHI::ShaderResourceGroup> m_srg;

    Asset<ShaderAssetStorage> m_pixelShaderAsset;
    Asset<ShaderAssetStorage> m_vertexShaderAsset;

    Rc<RHI::Buffer> m_constantBuffer;

    Asset<MeshAssetStorage> m_meshAsset;
    Asset<ImageAssetStorage> m_textureAsset;
    Rc<RHI::Sampler> m_textureSampler;

    Rc<RHI::IWindow> m_window;
    RHI::Viewport m_viewport;
    RHI::Scissor m_scissor;
};

int main(int argc, const char** argv)
{
    return ApplicationModule::Run<ExampleApplication>(argc, argv, [](ExampleApplication* app) {
        app->Initialize();
    });
}
