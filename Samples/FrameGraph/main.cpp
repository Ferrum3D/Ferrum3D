#include <FeCore/Console/Console.h>
#include <FeCore/DI/Activator.h>
#include <FeCore/IO/Path.h>
#include <FeCore/Math/Matrix4x4.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Modules/Configuration.h>
#include <FeCore/Time/DateTime.h>
#include <Framework/Application/Application.h>
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntitySystem.h>
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Entities/EntityWorldSystem.h>
#include <Framework/Module.h>
#include <Graphics/Assets/IModelAssetManager.h>
#include <Graphics/Assets/ITextureAssetManager.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Module.h>
#include <Graphics/Core/PipelineFactory.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Passes/Tools/Blit.h>
#include <Graphics/Passes/Tools/Downsample.h>
#include <Graphics/Module.h>

using namespace FE;
using namespace FE::Graphics;

inline constexpr const char* kExampleName = "Ferrum3D - FrameGraph Sample";


namespace
{
    struct TestPositionComponent final
    {
        PackedVector3F m_position;
    };

    struct TestRotationComponent final
    {
        Quaternion m_rotation;

        void Init()
        {
            m_rotation = Quaternion::RotationY(Constants::kPI * 0.5f);
        }

        void Shutdown() {}
    };

    struct TestScaleComponent final
    {
        float m_scale;

        void Init()
        {
            m_scale = 1.0f;
        }

        void Shutdown() {}
    };

    struct TestTransformComponent final
    {
        Matrix4x4 m_transform;
    };


    struct TestEntitySystem final : public Framework::EntitySystem
    {
        void OnArchetypeChanged(const Framework::IComponentProvider* componentProvider,
                                [[maybe_unused]] const festd::span<const Framework::ComponentTypeID> addedComponents,
                                [[maybe_unused]] const festd::span<const Framework::ComponentTypeID> removedComponents) override
        {
            m_positionComponent = componentProvider->GetRequiredComponent<TestPositionComponent>();
            m_rotationComponent = componentProvider->SafeGetComponent<TestRotationComponent>();
            m_scaleComponent = componentProvider->SafeGetComponent<TestScaleComponent>();
            m_transformComponent = componentProvider->GetRequiredComponent<TestTransformComponent>();
        }

        void Update([[maybe_unused]] const Framework::EntityUpdateContext& context) override
        {
            Matrix4x4 transform = Matrix4x4::Translation(Vector3(m_positionComponent->m_position));
            if (m_rotationComponent)
                transform = transform * Matrix4x4::Rotation(m_rotationComponent->m_rotation);
            if (m_scaleComponent)
                transform = transform * Matrix4x4::Scale(Vector3(m_scaleComponent->m_scale));
            m_transformComponent->m_transform = transform;
        }

        TestPositionComponent* m_positionComponent = nullptr;
        TestRotationComponent* m_rotationComponent = nullptr;
        TestScaleComponent* m_scaleComponent = nullptr;
        TestTransformComponent* m_transformComponent = nullptr;
    };


    struct TestEntityWorldSystem final : public Framework::EntityWorldSystem
    {
        void RegisterArchetype(const Framework::Archetype* archetype) override
        {
            if (archetype->MatchesAll<TestPositionComponent, TestTransformComponent>())
                m_archetypes.push_back(archetype);
        }

        void UnregisterArchetype(const Framework::Archetype* archetype) override
        {
            const auto it = festd::find(m_archetypes, archetype);
            if (it != m_archetypes.end())
                m_archetypes.erase_unsorted(it);
        }

        void Update([[maybe_unused]] const Framework::EntityUpdateContext& context) override
        {
            for (const Framework::Archetype* archetype : m_archetypes)
            {
                for (const Framework::ArchetypeChunk* chunk : archetype->m_chunks)
                {
                    const auto* positionComponents = chunk->SafeGetComponentArray<TestPositionComponent>();
                    const auto* rotationComponents = chunk->SafeGetComponentArray<TestRotationComponent>();
                    const auto* scaleComponents = chunk->SafeGetComponentArray<TestScaleComponent>();
                    auto* transformComponents = chunk->SafeGetComponentArray<TestTransformComponent>();

                    for (uint32_t i = 0; i < chunk->m_entityCount; ++i)
                    {
                        Matrix4x4 transform = Matrix4x4::Translation(Vector3(positionComponents[i].m_position));
                        if (rotationComponents)
                            transform = transform * Matrix4x4::Rotation(rotationComponents[i].m_rotation);
                        if (scaleComponents)
                            transform = transform * Matrix4x4::Scale(Vector3(scaleComponents[i].m_scale));
                        transformComponents[i].m_transform = transform;
                    }
                }
            }
        }

        festd::vector<const Framework::Archetype*> m_archetypes;
    };


    struct ComputePipeline final : public Core::ComputePipelineVariantSet
    {
        FE_DECLARE_PIPELINE_SET(ComputePipeline);

    private:
        void SetupRequest(uint32_t, Core::ComputePipelineRequest& request) override
        {
            request
                .m_desc //
                .SetComputeShader("Shader.cs.hlsl");
        }
    };
    FE_IMPLEMENT_PIPELINE_SET(ComputePipeline);


    struct MeshShaderPipeline final : public Core::GraphicsPipelineVariantSet
    {
        FE_SHADER_SPEC(ColorTargetFormat, Core::Format::kB8G8R8A8_SRGB, Core::Format::kR8G8B8A8_SRGB,
                       Core::Format::kR8G8B8A8_UNORM, Core::Format::kB8G8R8A8_UNORM);
        using Specializer = Core::ShaderSpecializer<ColorTargetFormat>;

        FE_DECLARE_PIPELINE_SET(MeshShaderPipeline, Specializer);

    private:
        void SetupRequest(const uint32_t variantIndex, Core::GraphicsPipelineRequest& request) override
        {
            const Specializer specializer(variantIndex);

            request
                .m_desc //
                // .SetAmplificationShader("Shader.as.hlsl")
                .SetMeshShader("Shader.ms.hlsl")
                .SetPixelShader("ShaderMesh.ps.hlsl")
                .SetRTVFormat(specializer.Get<ColorTargetFormat>())
                .SetDSVFormat(Core::Format::kD32_SFLOAT_S8_UINT)
                .SetColorBlend(Core::TargetColorBlending::kDisabled)
                .SetDepthStencil(Core::DepthStencilState::kWriteIfGreater)
                .SetRasterization(Core::RasterizationState::kFillBackCull);
        }
    };
    FE_IMPLEMENT_PIPELINE_SET(MeshShaderPipeline);


    struct ExampleApplication final : public Framework::Application
    {
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

            m_entityWorld = festd::make_unique<Framework::EntityWorld>();

            {
                using namespace Framework;

                EntityRegistry* registry = m_entityWorld->GetPersistentRegistry();
                Entity* entity = registry->CreateEntity("Test");
                entity->AddComponent<TestPositionComponent>();
                entity->AddComponent<TestRotationComponent>();
                // entity->AddComponent<TestScaleComponent>();
                entity->AddComponent<TestTransformComponent>();

                // entity->AddSystem(Memory::DefaultNew<TestEntitySystem>());
                m_entityWorld->AddSystem(Memory::DefaultNew<TestEntityWorldSystem>());

                m_testEntity = entity;
            }

            m_factory = serviceProvider->ResolveRequired<Core::DeviceFactory>();

            for (const Core::AdapterInfo& adapterInfo : m_factory->EnumerateAdapters())
            {
                if (adapterInfo.m_kind == Core::AdapterKind::kDiscrete)
                {
                    m_factory->CreateDevice(adapterInfo.m_name);
                    break;
                }
            }

            m_device = serviceProvider->ResolveRequired<Core::Device>();
            m_copyQueue = serviceProvider->ResolveRequired<Core::AsyncCopyQueue>();

            const RectInt scissorRect = m_mainWindow->GetClientRect();

            m_viewport = serviceProvider->ResolveRequired<Core::Viewport>();
            m_viewport->Init({ static_cast<uint32_t>(scissorRect.Width()),
                               static_cast<uint32_t>(scissorRect.Height()),
                               m_mainWindow->GetNativeHandle().m_value });

            Core::CompileGlobalPipelineSets(serviceProvider->ResolveRequired<Core::PipelineFactory>());
            Core::WaitForGlobalPipelineSets();

            m_timer.Start();

            ITextureAssetManager* textureAssetManager = serviceProvider->ResolveRequired<ITextureAssetManager>();
            m_texture = textureAssetManager->Load("Textures/test2.ftx");
            m_texture->m_completionWaitGroup->Wait();
            FE_Assert(m_texture->m_status == AssetLoadingStatus::kCompletelyLoaded);

            IModelAssetManager* modelAssetManager = serviceProvider->ResolveRequired<IModelAssetManager>();
            m_model = modelAssetManager->Load("Models/StanfordBunny.fmd");
            // m_model = modelAssetManager->Load("Models/TheSphere.fmd");
            m_model->m_completionWaitGroup->Wait();
            FE_Assert(m_model->m_status == AssetLoadingStatus::kCompletelyLoaded);
        }

    private:
        struct ViewData final
        {
            Core::Texture* m_mainColorTarget = nullptr;
            Core::Texture* m_mainDepthTarget = nullptr;
        };

        struct PassData final
        {
            ModelAsset* m_model = nullptr;
            const Core::ComputePipeline* m_computePipeline = nullptr;
            const Core::GraphicsPipeline* m_meshShaderPipeline = nullptr;
        };

        void PrepareView(Core::Viewport* viewport, Core::FrameGraphBlackboard& blackboard)
        {
            FE_PROFILER_ZONE();

            ViewData& viewData = blackboard.Add<ViewData>();
            viewData.m_mainColorTarget = viewport->GetCurrentColorTarget();
            viewData.m_mainDepthTarget = m_mainDepthTarget.Get();
        }

        void Render(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
        {
            FE_PROFILER_ZONE();

            m_timer.Stop();

            const float deltaTime = static_cast<float>(m_timer.GetElapsedSeconds());
            m_timer.Start();

            Memory::FiberTempAllocator temp;

            const ViewData& viewData = blackboard.Get<ViewData>();

            MeshShaderPipeline::Specializer meshShaderPipelineSpecializer;
            meshShaderPipelineSpecializer.Set<MeshShaderPipeline::ColorTargetFormat>(
                viewData.m_mainColorTarget->GetDesc().m_imageFormat);

            PassData& passData = blackboard.Add<PassData>();
            passData.m_model = m_model.Get();
            passData.m_computePipeline = ComputePipeline::GetPipeline();
            passData.m_meshShaderPipeline = MeshShaderPipeline::GetPipeline(meshShaderPipelineSpecializer);

            Core::Buffer* instanceData = graph.GetResourcePool()->CreateStructuredBuffer<Matrix4x4>("InstanceDataBuffer", 3);

            static float rotation = 0.0f;
            rotation += deltaTime * Constants::kPI * 0.5f;
            rotation = Math::Fmod(rotation, Constants::kPI * 2.0f);

            for (uint32_t computePassIndex = 0; computePassIndex < 3; ++computePassIndex)
            {
                computePass.SetFunction([instanceData, viewport, computePassIndex](Core::FrameGraphContext& context) {
                    FE_PROFILER_ZONE();

                    auto& frameGraph = context.GetGraph();
                    auto& localBlackboard = frameGraph.GetBlackboard();
                    const auto& localPassData = localBlackboard.GetRequired<PassData>();

                    struct ShaderConstants final
                    {
                        Matrix4x4 m_matrix;
                        BufferUAVDescriptor m_instanceData;
                        uint32_t m_offset;
                        Vector2 m_padding;
                    };

                    const float aspectRatio = viewport.Width() / viewport.Height();

                    const auto worldMatrix = Matrix4x4::RotationX(Constants::kPI * 0.5f) * Matrix4x4::RotationY(rotation);
                    const auto invWorldMatrix = Matrix4x4::RotationY(-rotation) * Matrix4x4::RotationX(-Constants::kPI * 0.5f);

                    const auto viewMatrix = Matrix4x4::LookAt(Vector3(0.0f, 2.5f, -2.0f), Vector3::AxisY(), Vector3::AxisY());
                    const auto projectionMatrix = Matrix4x4::Projection(Constants::kPI * 0.3f, aspectRatio, 0.01f, 1000.0f);

                    ShaderConstants shaderConstants;
                    shaderConstants.m_offset = computePassIndex;

                    if (computePassIndex == 0)
                    {
                        shaderConstants.m_matrix = worldMatrix;
                    }
                    else if (computePassIndex == 1)
                    {
                        shaderConstants.m_matrix = viewMatrix * projectionMatrix;
                    }
                    else
                    {
                        shaderConstants.m_matrix = Math::Transpose(invWorldMatrix);
                    }

                    shaderConstants.m_instanceData = frameGraph.GetUAV(instanceData);
                    context.PushConstants(shaderConstants);
                    context.Dispatch(localPassData.m_computePipeline, 1);
                });
            }

            const auto graphicsPass = builder.AddPass("DrawTriangle");

            const Core::RenderTargetHandle intermediateColorTarget = graphicsPass.WriteRenderTarget(graphicsPass.CreateImage(
                "IntermediateColorTarget",
                Core::TextureDesc::Img2D(viewportDesc.m_width, viewportDesc.m_height, m_colorTargetFormat, false)));

            const Core::RenderTargetHandle colorTarget = graphicsPass.WriteRenderTarget(intermediateColorTarget);
            const Core::RenderTargetHandle depthTarget = graphicsPass.WriteRenderTarget(graph.GetMainDepthStencilTarget());

            instanceData = graphicsPass.Read(instanceData, Core::BufferReadType::kShaderResource);

            if (0)
            {
                graphicsPass.SetFunction([colorTarget, depthTarget, instanceData, viewport, t = m_texture->m_resource](
                                             Core::FrameGraphContext& context) {
                    FE_PROFILER_ZONE();

                    auto& frameGraph = context.GetGraph();
                    auto& localBlackboard = frameGraph.GetBlackboard();
                    const auto& localPassData = localBlackboard.GetRequired<PassData>();

                    struct ShaderConstants final
                    {
                        TextureSRVDescriptor m_textureSRV;
                        SamplerDescriptor m_sampler;
                        BufferSRVDescriptor m_instanceData;
                        uint32_t m_padding;
                    };

                    ShaderConstants shaderConstants;
                    shaderConstants.m_textureSRV = frameGraph.GetSRV(t.Get(), Core::TextureSubresource::kInvalid);
                    shaderConstants.m_sampler = frameGraph.GetSampler(Core::SamplerState::kLinearWrap);
                    shaderConstants.m_instanceData = frameGraph.GetSRV(instanceData);
                    context.PushConstants(shaderConstants);

                    context.SetRenderTarget(colorTarget, depthTarget);
                    context.SetRenderTargetLoadOperations(
                        Core::RenderTargetLoadOperations{}.ClearAll(Colors::kDarkSlateBlue, 0.0f, 0));
                    context.SetRenderTargetStoreOperations(Core::RenderTargetStoreOperations::kDefault);
                    context.SetViewport(viewport);
                    context.Draw(localPassData.m_drawCall);
                });
            }
            else
            {
                graphicsPass.SetFunction([colorTarget, depthTarget, instanceData, viewport, t = m_texture->m_resource](
                                             Core::FrameGraphContext& context) {
                    FE_PROFILER_ZONE();

                    auto& frameGraph = context.GetGraph();
                    auto& localBlackboard = frameGraph.GetBlackboard();
                    const auto& localPassData = localBlackboard.GetRequired<PassData>();

                    struct ShaderConstants final
                    {
                        ByteAddressBufferDescriptor m_geometry;
                        Texture2DDescriptor<Vector4> m_texture;
                        SamplerDescriptor m_sampler;
                        StructuredBufferDescriptor<Matrix4x4> m_instanceData;
                        Core::MeshLodInfo m_lodInfo;
                    };

                    ModelAsset* model = localPassData.m_model;

                    const uint32_t lodIndex = DateTime<TZ::Local>::Now().Second() % model->m_lodCount;
                    const Core::Buffer* geometryBuffer = model->GetGeometryBuffer(lodIndex);
                    const Core::MeshLodInfo lodInfo = model->GetLodInfo(0, lodIndex);

                    ShaderConstants shaderConstants;
                    shaderConstants.m_lodInfo = lodInfo;
                    shaderConstants.m_geometry = frameGraph.GetSRV(geometryBuffer);
                    shaderConstants.m_texture = frameGraph.GetSRV(t.Get(), Core::TextureSubresource::kInvalid);
                    shaderConstants.m_sampler = frameGraph.GetSampler(Core::SamplerState::kLinearWrap);
                    shaderConstants.m_instanceData = frameGraph.GetSRV(instanceData);
                    context.PushConstants(shaderConstants);

                    context.SetRenderTarget(colorTarget, depthTarget);
                    context.SetRenderTargetLoadOperations(
                        Core::RenderTargetLoadOperations{}.ClearAll(Colors::kDarkSlateBlue, 0.0f, 0));
                    context.SetRenderTargetStoreOperations(Core::RenderTargetStoreOperations::kDefault);
                    context.SetViewport(viewport);
                    // context.DispatchMesh(localPassData.m_meshShaderPipeline, Math::CeilDivide(lodInfo.m_meshletCount, 32));
                    context.DispatchMesh(localPassData.m_meshShaderPipeline, lodInfo.m_meshletCount);
                });
            }

            const auto colorTargetMips = Tools::Downsample::AddPass(builder, colorTarget);
            const auto finalColorTarget = Tools::Blit::AddPass(builder, colorTargetMips[1], graph.GetMainColorTarget());
            FE_Unused(finalColorTarget);
        }

        HighResolutionTimer m_timer;

        Core::AsyncCopyQueue* m_copyQueue = nullptr;

        Rc<WaitGroup> ScheduleUpdate() override
        {
            FE_PROFILER_ZONE();

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();

            m_entityWorld->Update();

            auto* positionComponent = m_testEntity->GetRequiredComponent<TestPositionComponent>();
            positionComponent->m_position = { 1.0f, 2.0f, 3.0f };

            m_frameGraph = serviceProvider->ResolveRequired<Core::FrameGraph>();
            m_frameGraph->BeginFrame();

            Core::FrameGraphBuilder builder{ m_frameGraph.Get() };
            PrepareView(m_viewport.Get());
            Render(*m_frameGraph, builder, m_frameGraph->GetBlackboard());

            m_frameGraph->CompileAndExecute();
            m_device->EndFrame();
            return nullptr;
        }

        festd::unique_ptr<Framework::EntityWorld> m_entityWorld;
        Framework::Entity* m_testEntity = nullptr;

        festd::unique_ptr<Framework::StdoutLogSink> m_logSink;
        Core::DeviceFactory* m_factory = nullptr;
        Core::Device* m_device = nullptr;
        Rc<TextureAsset> m_texture;
        Rc<ModelAsset> m_model;

        Rc<Core::Texture> m_mainDepthTarget;
        Rc<Core::Viewport> m_viewport;
        Rc<Core::FrameGraph> m_frameGraph;
    };
} // namespace

int main(const int32_t argc, const char** argv)
{
    Env::ApplicationInfo applicationInfo;
    applicationInfo.m_name = kExampleName;

    Graphics::Module::Init();
    Framework::Module::Init();
    Env::Init(applicationInfo);

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
    Env::Module::ShutdownModules();
    return exitCode;
}
