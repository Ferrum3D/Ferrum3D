#include <FeCore/Console/Console.h>
#include <FeCore/IO/Path.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Modules/Configuration.h>
#include <FeCore/Time/DateTime.h>
#include <Framework/Application/Application.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/DrawListBuilder.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/GeometryPool.h>
#include <Graphics/Core/InputLayoutBuilder.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/Viewport.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - FrameGraph Sample";


struct LogColorScope final
{
    explicit LogColorScope(const LogSeverity severity)
    {
        Console::Color color;
        switch (severity)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case LogSeverity::kWarning:
            color = Console::Color::kYellow;
            break;

        case LogSeverity::kError:
        case LogSeverity::kCritical:
            color = Console::Color::kRed;
            break;

        case LogSeverity::kTrace:
        case LogSeverity::kDebug:
            color = Console::Color::kAqua;
            break;

        case LogSeverity::kInfo:
            color = Console::Color::kLime;
            break;
        }

        Console::SetTextColor(color);
    }

    ~LogColorScope()
    {
        Console::SetTextColor(Console::Color::kDefault);
    }
};


struct StdoutLogSink final : public LogSinkBase
{
    StdoutLogSink(Logger* logger)
        : LogSinkBase(logger)
    {
    }

    void Log(const LogSeverity severity, const SourceLocation sourceLocation, const festd::string_view message) override
    {
        {
            const auto location = Fmt::FixedFormatSized<IO::kMaxPathLength + 16>(
                "{}({}): ", sourceLocation.m_fileName, sourceLocation.m_lineNumber);
            Console::Write(festd::string_view(location.data(), location.size()));
        }

        {
            const auto date = DateTime<TZ::Local>::Now().ToString(DateTimeFormatKind::kISO8601);
            Console::Write(date);
        }

        Console::Write(" [");

        {
            LogColorScope colorScope{ severity };
            Console::Write(LogSeverityToString(severity));
        }

        Console::Write("] ");

        Console::Write(festd::string_view(message.data(), message.size()));
        Console::Write("\n");

        if (severity >= LogSeverity::kWarning)
            Console::Flush();
    }
};


struct ExampleApplication final : public Framework::Application
{
    FE_RTTI_Class(ExampleApplication, "78304A61-C92E-447F-9834-4D547B1D950F");

    ExampleApplication(const int32_t argc, const char** argv)
    {
        for (int32_t argIndex = 1; argIndex < argc; ++argIndex)
            m_commandLine.push_back(argv[argIndex]);
    }

    ~ExampleApplication() override
    {
        m_device->WaitIdle();
    }

private:
    void InitializeApp() override
    {
        FE_PROFILER_ZONE();

        DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

        m_logSink = festd::make_unique<StdoutLogSink>(serviceProvider->ResolveRequired<Logger>());

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
    }

    struct PassProducer final : public Core::PassProducer
    {
        FE_RTTI_Class(PassProducer, "A22B22E2-2196-4439-8D70-AFB8E64379AD");

        struct PassData final
        {
            Core::DrawList m_drawList;
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
                .AddChannel(Core::VertexChannelFormat::kR32G32B32_SFLOAT, Core::ShaderSemantic::kColor);

            const Core::InputStreamLayout inputLayout = inputLayoutBuilder.Build();

            Core::GraphicsPipelineRequest pipelineRequest;
            pipelineRequest.m_desc.SetInputLayout(inputLayout)
                .SetPixelShader("Shader.ps.hlsl")
                .SetVertexShader("Shader.vs.hlsl")
                .SetRTVFormat(viewport->GetColorTargetFormat())
                .SetDSVFormat(viewport->GetDepthTargetFormat())
                .SetColorBlend(Core::TargetColorBlending::kDisabled)
                .SetDepthStencil(Core::DepthStencilState::kDisabled)
                .SetRasterization(Core::RasterizationState::kFillNoCull);

            m_pipeline = m_pipelineFactory->CreateGraphicsPipeline(pipelineRequest);

            Core::GeometryAllocationDesc geometryDesc;
            geometryDesc.m_name = "Triangle";
            geometryDesc.m_inputLayout = inputLayout;
            geometryDesc.m_indexType = Core::IndexType::kUint32;
            geometryDesc.m_indexCount = 0;
            geometryDesc.m_vertexCount = 3;
            m_geometry = m_geometryPool->Allocate(geometryDesc);
            m_geometryPool->GetAvailabilityWaitGroup(m_geometry)->Wait();

            struct Vertex final
            {
                PackedVector3F m_position;
                PackedVector3F m_color;
            };

            static const Vertex kVertexData[] = {
                { PackedVector3F(0.0f, -0.5f, 0.0f), PackedVector3F(1.0f, 0.3f, 0.3f) },
                { PackedVector3F(0.5f, 0.5f, 0.0f), PackedVector3F(0.3f, 1.0f, 0.3f) },
                { PackedVector3F(-0.5f, 0.5f, 0.0f), PackedVector3F(0.3f, 0.3f, 1.0f) },
            };

            const Core::GeometryView geometryView = m_geometryPool->GetView(m_geometry);

            Core::AsyncCopyCommandListBuilder copyCommandListBuilder{ &tempAllocator, 256 };
            copyCommandListBuilder.UploadBuffer(geometryView.m_streamBufferViews[0].m_buffer, kVertexData);

            Core::AsyncCopyCommandList copyCommandList = copyCommandListBuilder.Build();
            auto copyWaitGroup = m_copyQueue->ExecuteCommandList(copyCommandList);

            const Rc<WaitGroup> waitGroups[] = { m_pipeline->GetCompletionWaitGroup(), copyWaitGroup };
            WaitGroup::WaitAll(waitGroups);
            copyCommandList.Free();
        }

        void Setup(Core::FrameGraph& graph, Core::FrameGraphBuilder& builder, Core::FrameGraphBlackboard& blackboard) override
        {
            FE_PROFILER_ZONE();

            Memory::FiberTempAllocator tempAllocator;

            const auto pass = builder.AddPass("DrawTriangle");

            // Geometry views and draw arguments are copied into the FrameGraph's internal buffer
            // that lives until the rendering thread finishes. So we don't need to store this data in the
            // blackboard.

            const Core::GeometryView geometryView = m_geometryPool->GetView(m_geometry);

            Core::DrawCall drawCall;
            drawCall.InitForSingleInstance(&geometryView, m_pipeline.Get());

            Core::DrawListBuilder drawListBuilder{ graph.GetAllocator(), &tempAllocator };
            drawListBuilder.AddDrawCall(drawCall);

            PassData& passData = blackboard.Add<PassData>();
            passData.m_drawList = drawListBuilder.Build();

            const Core::ViewportDesc& viewportDesc = graph.GetViewport()->GetDesc();
            const RectF viewport{ 0, 0, static_cast<float>(viewportDesc.m_width), static_cast<float>(viewportDesc.m_height) };

            const Core::ImageHandle colorTarget = pass.Write(graph.GetRenderTarget(), Core::ImageWriteType::kColorTarget);
            const Core::ImageHandle depthTarget = pass.Write(graph.GetDepthStencil(), Core::ImageWriteType::kDepthStencilTarget);

            pass.SetFunction([colorTarget, depthTarget, viewport](Core::FrameGraphContext* context) {
                FE_PROFILER_ZONE();

                auto& localBlackboard = context->GetFrameGraph()->GetBlackboard();
                const auto& localPassData = localBlackboard.GetRequired<PassData>();

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

        Rc<Core::GraphicsPipeline> m_pipeline;
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

    void RegisterServices(DI::ServiceRegistryBuilder builder) override
    {
        builder.Bind<Env::Configuration>()
            .ToFunc([this](DI::IServiceProvider*, Memory::RefCountedObjectBase** result) {
                std::pmr::memory_resource* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
                *result = Memory::New<Env::Configuration>(allocator, m_commandLine);
                return DI::ResultCode::kSuccess;
            })
            .InSingletonScope();
    }

    void LoadModules(ModuleLoadingList& modules) override
    {
        modules.Add<Framework::FrameworkModule>();
        modules.Add<Core::GraphicsCoreModule>();
    }

    festd::unique_ptr<StdoutLogSink> m_logSink;
    festd::vector<festd::string_view> m_commandLine;
    Rc<Core::DeviceFactory> m_factory;
    Rc<Core::Device> m_device;

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
        application->Initialize();
        exitCode = application->Run();
    });

    mainJob.Schedule(jobSystem, FiberAffinityMask::kMainThread);
    jobSystem->Start();

    Memory::Delete(allocator, application);
    return exitCode;
}
