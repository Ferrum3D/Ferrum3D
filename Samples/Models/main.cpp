#include <FeCore/Assets/Asset.h>
#include <FeCore/Modules/ApplicationModule.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <Graphics/Assets/ImageAssetStorage.h>
#include <Graphics/Assets/MeshAssetStorage.h>
#include <Graphics/Assets/ShaderAssetStorage.h>
#include <Graphics/Module.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/CommandList.h>
#include <Graphics/Core/CommandQueue.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/Framebuffer.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/IWindow.h>
#include <Graphics/Core/ImageView.h>
#include <Graphics/Core/InputLayoutBuilder.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/RenderPass.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/ShaderCompiler.h>
#include <Graphics/Core/ShaderModule.h>
#include <Graphics/Core/ShaderReflection.h>
#include <Graphics/Core/ShaderResourceGroup.h>
#include <Graphics/Core/Swapchain.h>

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

        m_factory = pServiceProvider->ResolveRequired<Core::DeviceFactory>();
        m_device = pServiceProvider->ResolveRequired<Core::Device>();

        const Core::AdapterInfo adapterInfo = m_factory->EnumerateAdapters()[0];
        m_factory->CreateDevice(adapterInfo.m_name);

        m_graphicsQueue = m_device->GetCommandQueue(Core::HardwareQueueKindFlags::kGraphics);
        m_transferQueue = m_device->GetCommandQueue(Core::HardwareQueueKindFlags::kTransfer);

        const Rc resourcePool = pServiceProvider->ResolveRequired<Core::ResourcePool>();
        const Rc pipelineFactory = pServiceProvider->ResolveRequired<Core::PipelineFactory>();

        m_window = pServiceProvider->ResolveRequired<Core::IWindow>();
        m_window->Init(Core::WindowDesc{ 800, 600, kExampleName });
        m_viewport = m_window->CreateViewport();
        m_scissor = m_window->CreateScissor();

        Core::SwapchainDesc swapchainDesc{};
        swapchainDesc.m_frameCount = 2;
        swapchainDesc.m_imageWidth = m_scissor.Width();
        swapchainDesc.m_imageHeight = m_scissor.Height();
        swapchainDesc.m_nativeWindowHandle = m_window->GetNativeHandle();
        swapchainDesc.m_queue = m_graphicsQueue.Get();
        swapchainDesc.m_verticalSync = true;
        m_swapChain = pServiceProvider->ResolveRequired<Core::Swapchain>();
        m_swapChain->Init(swapchainDesc);

        m_meshAsset = Asset<MeshAssetStorage>::LoadSynchronously(assetManager.Get(), "Models/cube");
        m_textureAsset = Asset<ImageAssetStorage>::LoadSynchronously(assetManager.Get(), "Textures/image");
        m_textureSampler = pServiceProvider->ResolveRequired<Core::Sampler>();
        m_textureSampler->Init(Core::SamplerDesc{});

        {
            const float imageWidth = static_cast<float>(m_swapChain->GetDesc().m_imageWidth);
            const float imageHeight = static_cast<float>(m_swapChain->GetDesc().m_imageHeight);
            const float aspectRatio = imageWidth / imageHeight;

            auto constantData = Matrix4x4F::Identity();
            constantData = constantData * Matrix4x4F::Projection(Constants::PI * 0.5, aspectRatio, 0.1f, 10.0f);
            constantData = constantData * Matrix4x4F::RotationY(Constants::PI);
            constantData = constantData * Matrix4x4F::RotationX(-0.5f);
            constantData = constantData * Matrix4x4F::Translation(Vector3F(0.0f, 0.8f, -1.5f) * 2);
            constantData = constantData * Matrix4x4F::RotationY(Constants::PI * -1.3f);

            const auto constantBufferDesc =
                Core::BufferDesc(sizeof(constantData), Core::BindFlags::kConstantBuffer, Core::ResourceUsage::kHostWriteThrough);
            m_constantBuffer = resourcePool->CreateBuffer("Constant Buffer", constantBufferDesc).value();
            m_constantBuffer->UpdateData(constantData.RowMajorData());
        }

        m_vertexShaderAsset = Asset<ShaderAssetStorage>::LoadSynchronously(assetManager.Get(), "Shaders/Shader.vs");
        m_pixelShaderAsset = Asset<ShaderAssetStorage>::LoadSynchronously(assetManager.Get(), "Shaders/Shader.ps");

        Core::RenderPassDesc renderPassDesc{};

        Core::AttachmentDesc attachmentDesc{};
        attachmentDesc.m_format = m_swapChain->GetDesc().m_format;
        attachmentDesc.m_initialState = Core::ResourceState::kUndefined;
        attachmentDesc.m_finalState = Core::ResourceState::kPresent;

        Core::AttachmentDesc depthAttachmentDesc{};
        depthAttachmentDesc.m_format = m_swapChain->GetDSV()->GetDesc().m_format;
        depthAttachmentDesc.m_storeOp = Core::AttachmentStoreOp::kStore;
        depthAttachmentDesc.m_loadOp = Core::AttachmentLoadOp::kClear;
        depthAttachmentDesc.m_initialState = Core::ResourceState::kUndefined;
        depthAttachmentDesc.m_finalState = Core::ResourceState::kDepthWrite;

        const std::array attachments{ attachmentDesc, depthAttachmentDesc };
        renderPassDesc.m_attachments = attachments;

        Core::SubpassDesc subpassDesc{};
        const Core::SubpassAttachment renderTargetAttachment{ Core::ResourceState::kRenderTarget, 0 };
        subpassDesc.m_renderTargetAttachments = festd::span{ &renderTargetAttachment, 1 };
        subpassDesc.m_depthStencilAttachment = Core::SubpassAttachment{ Core::ResourceState::kDepthWrite, 1 };
        renderPassDesc.m_subpasses = festd::span(&subpassDesc, 1);
        Core::SubpassDependency dependency{};
        renderPassDesc.m_subpassDependencies = festd::span(&dependency, 1);

        m_renderPass = pServiceProvider->ResolveRequired<Core::RenderPass>();
        m_renderPass->Init(renderPassDesc);

        Core::ShaderModule* psModule = m_pixelShaderAsset->GetShaderModule();
        Core::ShaderModule* vsModule = m_vertexShaderAsset->GetShaderModule();
        const Core::ShaderReflection* psReflection = psModule->GetReflection();
        const Core::ShaderReflection* vsReflection = vsModule->GetReflection();

        m_srg = pServiceProvider->ResolveRequired<Core::ShaderResourceGroup>();

        const std::array shadersReflection{ psReflection, vsReflection };
        m_srg->Init({ shadersReflection });

        Core::ShaderResourceGroupData srgData{};
        srgData.Set(psReflection->GetResourceBindingIndex("g_Sampler"), m_textureSampler.Get());
        srgData.Set(psReflection->GetResourceBindingIndex("g_Texture"), m_textureAsset->GetImageView());
        srgData.Set(vsReflection->GetResourceBindingIndex("Settings"), m_constantBuffer.Get());
        m_srg->Update(srgData);

        Core::GraphicsPipelineDesc pipelineDesc{};
        pipelineDesc.m_inputLayout = Core::InputLayoutBuilder(Core::PrimitiveTopology::kTriangleList)
                                         .AddBuffer(Core::InputStreamRate::kPerVertex)
                                         .AddAttribute(Core::Format::kR32G32B32_SFLOAT, "POSITION")
                                         .AddAttribute(Core::Format::kR32G32_SFLOAT, "TEXCOORD")
                                         .Build()
                                         .Build();

        pipelineDesc.m_renderPass = m_renderPass.Get();
        pipelineDesc.m_subpassIndex = 0;
        pipelineDesc.m_colorBlend.m_targetBlendStates[0] = Core::TargetColorBlending::kDisabled;

        const std::array shaders{ psModule, vsModule };
        pipelineDesc.m_shaders = festd::span(shaders);

        const std::array srgs{ m_srg.Get() };
        pipelineDesc.m_shaderResourceGroups = srgs;
        pipelineDesc.m_viewport = m_viewport;
        pipelineDesc.m_scissor = m_scissor;
        pipelineDesc.m_rasterization = Core::RasterizationState::kDefaultBackCull;
        pipelineDesc.m_depthStencil = Core::DepthStencilState::kEnabled;

        m_pipeline = pipelineFactory->CreateGraphicsPipeline("GraphicsPSO", pipelineDesc).value();

        m_fence = pServiceProvider->ResolveRequired<Core::Fence>();
        m_fence->Init();
        m_fenceValue = 0;

        for (uint32_t i = 0; i < m_swapChain->GetDesc().m_frameCount; ++i)
            m_fenceValues.push_back(0);

        const festd::span rtvSpan = m_swapChain->GetRTVs();
        m_rTVs.resize(rtvSpan.size());
        Memory::Copy(rtvSpan, festd::span(m_rTVs));

        for (uint32_t i = 0; i < m_swapChain->GetImageCount(); ++i)
        {
            Core::FramebufferDesc framebufferDesc{};
            framebufferDesc.m_renderPass = m_renderPass.Get();
            const auto views = eastl::vector{ m_rTVs[i], m_swapChain->GetDSV() };
            framebufferDesc.m_renderTargetViews = views;
            framebufferDesc.m_width = m_scissor.Width();
            framebufferDesc.m_height = m_scissor.Height();

            const Rc framebuffer = pServiceProvider->ResolveRequired<Core::Framebuffer>();
            framebuffer->Init(framebufferDesc);
            m_framebuffers.push_back(framebuffer);

            const Rc cmd = pServiceProvider->ResolveRequired<Core::CommandList>();
            cmd->Init({ Core::HardwareQueueKindFlags::kGraphics, Core::CommandListFlags::kNone });
            m_commandLists.push_back(cmd);

            const std::array clearValues{ Core::ClearValueDesc::CreateColorValue(Colors::kMediumAquamarine),
                                          Core::ClearValueDesc::CreateDepthStencilValue() };

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

    void Tick(const float deltaTimte) override
    {
        m_window->PollEvents();
        if (m_window->CloseRequested())
            return;

        const uint32_t frameIndex = m_frameIndex % m_fenceValues.size();
        m_fence->Wait(m_fenceValues[frameIndex]);
        m_swapChain->BeginFrame({ m_fence, ++m_fenceValue });

        const std::array commandLists{ m_commandLists[frameIndex].Get() };

        m_fenceValues[frameIndex] = ++m_fenceValue;
        m_graphicsQueue->Execute(commandLists);
        m_graphicsQueue->SignalFence({ m_fence, m_fenceValue });
        m_swapChain->Present({ m_fence, m_fenceValue });
    }

    ModuleDependency<Core::GraphicsCoreModule> m_rhiModule;
    ModuleDependency<GraphicsModule> m_graphicsModule;

    Rc<Core::DeviceFactory> m_factory;
    Rc<Core::Device> m_device;

    Rc<Core::Fence> m_fence;
    uint64_t m_fenceValue = 0;
    festd::vector<uint64_t> m_fenceValues;

    festd::vector<Rc<Core::Framebuffer>> m_framebuffers;
    festd::vector<Rc<Core::CommandList>> m_commandLists;
    Rc<Core::CommandQueue> m_graphicsQueue;
    Rc<Core::CommandQueue> m_transferQueue;

    Rc<Core::RenderPass> m_renderPass;
    Rc<Core::Swapchain> m_swapChain;
    Rc<Core::GraphicsPipeline> m_pipeline;
    festd::vector<Core::ImageView*> m_rTVs;

    Rc<Core::ShaderResourceGroup> m_srg;

    Asset<ShaderAssetStorage> m_pixelShaderAsset;
    Asset<ShaderAssetStorage> m_vertexShaderAsset;

    Rc<Core::Buffer> m_constantBuffer;

    Asset<MeshAssetStorage> m_meshAsset;
    Asset<ImageAssetStorage> m_textureAsset;
    Rc<Core::Sampler> m_textureSampler;

    Rc<Core::IWindow> m_window;
    Core::Viewport m_viewport;
    Core::Scissor m_scissor;
};

int main(int argc, const char** argv)
{
    return ApplicationModule::Run<ExampleApplication>(argc, argv, [](ExampleApplication* app) {
        app->Initialize();
    });
}
