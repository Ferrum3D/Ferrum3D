#include <FeCore/Console/Console.h>
#include <FeCore/IO/Path.h>
#include <FeCore/Math/Matrix4x4F.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Modules/Configuration.h>
#include <FeCore/Time/DateTime.h>
#include <Framework/Application/Application.h>
#include <Graphics/Assets/ITextureAssetManager.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/DrawListBuilder.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/GeometryPool.h>
#include <Graphics/Core/InputLayoutBuilder.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Module.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - FrameGraph Sample";


struct ExampleApplication final : public Framework::Application
{
    FE_RTTI_Class(ExampleApplication, "78304A61-C92E-447F-9834-4D547B1D950F");

    ExampleApplication(const int32_t argc, const char** argv)
        : Application(argc, argv)
    {
    }

    ~ExampleApplication() override
    {
        m_device->WaitIdle();
    }

    void InitializeApp()
    {
        FE_PROFILER_ZONE();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

        m_logSink = festd::make_unique<Framework::StdoutLogSink>(serviceProvider->ResolveRequired<Logger>());

        m_factory = serviceProvider->ResolveRequired<Core::DeviceFactory>();

        for (const Core::AdapterInfo& adapterInfo : m_factory->EnumerateAdapters())
        {
            if (adapterInfo.m_kind == Core::AdapterKind::kDiscrete)
            {
                FE_Verify(m_factory->CreateDevice(adapterInfo.m_name) == Core::ResultCode::kSuccess);
                break;
            }
        }

        m_device = serviceProvider->ResolveRequired<Core::Device>();

        const RectInt scissorRect = m_mainWindow->GetClientRect();

        m_viewport = serviceProvider->ResolveRequired<Core::Viewport>();
        m_viewport->Init({ static_cast<uint32_t>(scissorRect.Width()),
                           static_cast<uint32_t>(scissorRect.Height()),
                           m_mainWindow->GetNativeHandle().m_value });

        m_passProducer = DI::DefaultNew<PassProducer>().value();
        m_passProducer->Init(m_viewport.Get());

        ITextureAssetManager* textureAssetManager = serviceProvider->ResolveRequired<ITextureAssetManager>();
        m_texture = textureAssetManager->Load("Textures/test1.ftx");
        m_texture->m_completionWaitGroup->Wait();
        FE_Assert(m_texture->m_status == AssetLoadingStatus::kCompletelyLoaded);

        m_passProducer->m_texture = m_texture->m_resource;
    }

private:
    struct PassProducer final : public Core::PassProducer
    {
        FE_RTTI_Class(PassProducer, "A22B22E2-2196-4439-8D70-AFB8E64379AD");

        struct PassData final
        {
            Core::DrawList m_drawList;
            Core::ComputePipeline* m_computePipeline = nullptr;
        };

        PassProducer(Core::PipelineFactory* pipelineFactory, Core::AsyncCopyQueue* copyQueue, Core::GeometryPool* geometryPool)
            : m_pipelineFactory(pipelineFactory)
            , m_copyQueue(copyQueue)
            , m_geometryPool(geometryPool)
        {
        }

        void Init(Core::Viewport* viewport)
        {
            FE_PROFILER_ZONE();

            Memory::FiberTempAllocator tempAllocator;

            Core::InputLayoutBuilder inputLayoutBuilder;
            inputLayoutBuilder.AddStream(Core::InputStreamRate::kPerVertex)
                .AddChannel(Core::VertexChannelFormat::kR32G32B32_SFLOAT, Core::ShaderSemantic::kPosition)
                .AddChannel(Core::VertexChannelFormat::kR32G32_SFLOAT, Core::ShaderSemantic::kTexCoord);

            const Core::InputStreamLayout inputLayout = inputLayoutBuilder.Build();

            Core::GraphicsPipelineRequest graphicsPipelineRequest;
            graphicsPipelineRequest.m_desc.SetInputLayout(inputLayout)
                .SetPixelShader("Shader.ps.hlsl")
                .SetVertexShader("Shader.vs.hlsl")
                .SetRTVFormat(viewport->GetColorTargetFormat())
                .SetDSVFormat(Core::Format::kD32_SFLOAT_S8_UINT)
                .SetColorBlend(Core::TargetColorBlending::kDisabled)
                .SetDepthStencil(Core::DepthStencilState::kDisabled)
                .SetRasterization(Core::RasterizationState::kFillNoCull);

            m_graphicsPipeline = m_pipelineFactory->CreateGraphicsPipeline(graphicsPipelineRequest);

            Core::ComputePipelineRequest computePipelineRequest;
            computePipelineRequest.m_desc.SetComputeShader("Shader.cs.hlsl");

            m_computePipeline = m_pipelineFactory->CreateComputePipeline(computePipelineRequest);

            struct Vertex final
            {
                PackedVector3F m_position;
                Vector2F m_uv;
            };

            static const Vertex kVertexData[] = {
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
                { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
                { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            };

            static const uint16_t kIndexData[] = {
                0, 1, 2, 2, 3, 0,
            };

            Core::GeometryAllocationDesc geometryDesc;
            geometryDesc.m_name = "Triangle";
            geometryDesc.m_inputLayout = inputLayout;
            geometryDesc.m_indexType = Core::IndexType::kUint16;
            geometryDesc.m_indexCount = festd::size(kIndexData);
            geometryDesc.m_vertexCount = festd::size(kVertexData);
            m_geometry = m_geometryPool->Allocate(geometryDesc);
            m_geometryPool->GetAvailabilityWaitGroup(m_geometry)->Wait();

            const Core::GeometryView geometryView = m_geometryPool->GetView(m_geometry);

            Core::AsyncCopyCommandListBuilder copyCommandListBuilder{ &tempAllocator, 256 };
            copyCommandListBuilder.UploadBuffer(geometryView.m_streamBufferViews[0].m_buffer, kVertexData);
            copyCommandListBuilder.UploadBuffer(geometryView.m_indexBufferView.m_buffer, kIndexData);

            const Rc copyWaitGroup = WaitGroup::Create();
            Core::AsyncCopyCommandList copyCommandList = copyCommandListBuilder.Build(copyWaitGroup.Get());

            m_copyQueue->ExecuteCommandList(&copyCommandList);

            WaitGroup::WaitAll({ m_graphicsPipeline->GetCompletionWaitGroup(),
                                 m_computePipeline->GetCompletionWaitGroup(),
                                 copyWaitGroup.Get() });
            copyCommandList.Free();
        }

        void Setup(Core::FrameGraph& graph, Core::FrameGraphBuilder& builder, Core::FrameGraphBlackboard& blackboard) override
        {
            FE_PROFILER_ZONE();

            Memory::FiberTempAllocator temp;

            // Geometry views and draw arguments are copied into the FrameGraph's internal buffer
            // that lives until the rendering thread finishes. So we don't need to store this data in the
            // blackboard.

            const Core::GeometryView geometryView = m_geometryPool->GetView(m_geometry);

            Core::DrawCall drawCall;
            drawCall.InitForSingleInstance(&geometryView, m_graphicsPipeline.Get());

            Core::DrawListBuilder drawListBuilder{ graph.GetAllocator(), &temp };
            drawListBuilder.AddDrawCall(drawCall);

            PassData& passData = blackboard.Add<PassData>();
            passData.m_drawList = drawListBuilder.Build();
            passData.m_computePipeline = m_computePipeline.Get();

            const Core::ViewportDesc& viewportDesc = graph.GetViewport()->GetDesc();
            const RectF viewport = viewportDesc.GetRect();

            const auto computePass = builder.AddPass("TestCompute");

            Core::BufferHandle instanceData = computePass.CreateStructuredBuffer<Matrix4x4F>("InstanceDataBuffer", 1);
            instanceData = computePass.Write(instanceData);

            computePass.SetFunction([instanceData](Core::FrameGraphContext* context) {
                FE_PROFILER_ZONE();

                auto* frameGraph = context->GetFrameGraph();
                auto& localBlackboard = frameGraph->GetBlackboard();
                const auto& localPassData = localBlackboard.GetRequired<PassData>();

                struct ShaderConstants final
                {
                    Matrix4x4F m_worldMatrix;
                    Core::BufferUAVDescriptor m_instanceData;
                    PackedVector3F m_padding;
                };

                ShaderConstants shaderConstants;
                shaderConstants.m_worldMatrix = Matrix4x4F::RotationZ(Constants::PI * 0.2f);
                shaderConstants.m_instanceData = frameGraph->GetUAV(instanceData);
                context->SetRootConstants(shaderConstants);
                context->Dispatch(localPassData.m_computePipeline, 1);
            });

            const auto graphicsPass = builder.AddPass("DrawTriangle");

            const Core::RenderTargetHandle colorTarget = graphicsPass.Write(graph.GetMainColorTarget());
            const Core::RenderTargetHandle depthTarget = graphicsPass.Write(graph.GetMainDepthStencilTarget());

            instanceData = graphicsPass.Read(instanceData, Core::BufferReadType::kShaderResource);

            graphicsPass.SetFunction(
                [colorTarget, depthTarget, instanceData, viewport, t = m_texture](Core::FrameGraphContext* context) {
                    FE_PROFILER_ZONE();

                    auto* frameGraph = context->GetFrameGraph();
                    auto& localBlackboard = frameGraph->GetBlackboard();
                    const auto& localPassData = localBlackboard.GetRequired<PassData>();

                    struct ShaderConstants final
                    {
                        Core::ImageSRVDescriptor m_textureSRV;
                        Core::SamplerDescriptor m_sampler;
                        Core::BufferSRVDescriptor m_instanceData;
                        uint32_t m_padding;
                    };

                    ShaderConstants shaderConstants;
                    shaderConstants.m_textureSRV = frameGraph->GetSRV(t.Get(), Core::ImageSubresource::kInvalid);
                    shaderConstants.m_sampler = frameGraph->GetSampler(Core::SamplerState::kLinearWrap);
                    shaderConstants.m_instanceData = frameGraph->GetSRV(instanceData);
                    context->SetRootConstants(shaderConstants);

                    context->SetRenderTarget(colorTarget, depthTarget);
                    context->SetRenderTargetLoadOperations(
                        Core::RenderTargetLoadOperations{}.ClearAll(Colors::kDarkSlateBlue, 0.0f, 0));
                    context->SetRenderTargetStoreOperations(Core::RenderTargetStoreOperations::kDefault);
                    context->SetViewport(viewport);
                    context->Draw(localPassData.m_drawList);
                });
        }

        Rc<Core::PipelineFactory> m_pipelineFactory;
        Rc<Core::AsyncCopyQueue> m_copyQueue;
        Rc<Core::GeometryPool> m_geometryPool;
        Core::GeometryHandle m_geometry;

        Rc<Core::Texture> m_texture;
        Rc<Core::GraphicsPipeline> m_graphicsPipeline;
        Rc<Core::ComputePipeline> m_computePipeline;
    };

    Rc<WaitGroup> ScheduleUpdate() override
    {
        FE_PROFILER_ZONE();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

        m_frameGraph = serviceProvider->ResolveRequired<Core::FrameGraph>();
        m_frameGraph->RegisterViewport(m_viewport.Get());
        m_frameGraph->AddPassProducer(m_passProducer.Get());
        m_frameGraph->Execute();
        m_device->EndFrame();
        return nullptr;
    }

    void LoadModules(ModuleLoadingList& modules) override
    {
        modules.Add<Framework::FrameworkModule>();
        modules.Add<Core::GraphicsCoreModule>();
        modules.Add<GraphicsModule>();
    }

    festd::unique_ptr<Framework::StdoutLogSink> m_logSink;
    Rc<Core::DeviceFactory> m_factory;
    Rc<Core::Device> m_device;
    Rc<TextureAsset> m_texture;

    Rc<Core::Viewport> m_viewport;
    Rc<Core::FrameGraph> m_frameGraph;
    Rc<PassProducer> m_passProducer;
};

int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = kExampleName;
    Env::CreateEnvironment(applicationInfo);

    std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);

    auto* application = Memory::New<ExampleApplication>(allocator, argc, argv);
    application->InitializeCore();

    IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();

    int32_t exitCode = 0;
    FunctorJob mainJob([application, &exitCode] {
        application->InitializeWindow();
        application->InitializeApp();
        exitCode = application->Run();
    });

    mainJob.Schedule(jobSystem, FiberAffinityMask::kMainThread);
    jobSystem->Start();

    Memory::Delete(allocator, application);
    return exitCode;
}
