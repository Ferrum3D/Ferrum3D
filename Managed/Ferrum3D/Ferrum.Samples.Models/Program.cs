using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Containers;
using Ferrum.Core.EventBus;
using Ferrum.Core.Framework;
using Ferrum.Core.Math;
using Ferrum.Core.Utils;
using Ferrum.Osmium.Assets;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.Descriptors;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.PipelineStates;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.VertexInput;
using Ferrum.Osmium.GPU.WindowSystem;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Samples.Models
{
    internal class ExampleApplication : ApplicationFramework
    {
        public const string ApplicationName = "Ferrum3D - Models";

        private Instance instance;
        private Adapter adapter;
        private Device device;
        private CommandQueue graphicsQueue;
        private Window window;
        private SwapChain swapChain;
        private Buffer constantBuffer;
        private Buffer indexBuffer;
        private Buffer vertexBuffer;
        private DisposableList<Fence> fences;
        private ShaderModule pixelShader;
        private ShaderModule vertexShader;
        private RenderPass renderPass;
        private GraphicsPipeline pipeline;
        private DisposableList<Framebuffer> framebuffers;
        private DisposableList<CommandBuffer> commandBuffers;
        private CommandQueue commandQueue;
        private Image textureImage;
        private Sampler textureSampler;
        private DescriptorAllocator descriptorAllocator;
        private DescriptorTable descriptorTable;

        protected override bool CloseEventReceived => window.CloseRequested;

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            var gpuModule = GetDependency<OsmiumGpuModule>();
            gpuModule.Initialize(new OsmiumGpuModule.Desc(ApplicationName, GraphicsApi.Vulkan));
            var assetsModule = GetDependency<OsmiumAssetsModule>();
            assetsModule.Initialize(new OsmiumAssetsModule.Desc());

            using var imageAsset = Asset.Load<ImageAsset>(Uuid.Parse("94FC6391-4656-4BE7-844D-8D87680A00F1"));
            using var meshAsset = Asset.Load<MeshAsset>(Uuid.Parse("884FEDDD-141D-49A0-92B2-38B519403D0A"));

            instance = gpuModule.CreateInstance();
            adapter = instance.Adapters.First();
            device = adapter.CreateDevice();

            graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, "Ferrum3D - Models");
            window = device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(window, graphicsQueue);
            swapChain = device.CreateSwapChain(swapChainDesc);

            var constantData = Matrix4x4F.Identity;
            constantData *= Matrix4x4F.CreateProjection(MathF.PI * 0.5f, swapChain.AspectRatio, 0.1f, 10.0f);
            constantData *= Matrix4x4F.CreateRotationY(MathF.PI);
            constantData *= Matrix4x4F.CreateRotationX(-0.5f);
            constantData *= Matrix4x4F.CreateTranslation(new Vector3F(0.0f, 0.8f, -1.5f) * 2);
            constantData *= Matrix4x4F.CreateRotationY(MathF.PI * -1.3f);

            constantBuffer =
                CreateHostVisibleBuffer(BindFlags.ConstantBuffer, device, new[] { constantData });

            vertexBuffer = device.CreateBuffer(BindFlags.VertexBuffer, meshAsset.VertexSize);
            vertexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            indexBuffer = device.CreateBuffer(BindFlags.IndexBuffer, meshAsset.IndexSize);
            indexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            textureImage = device.CreateImage(Image.Desc.Img2D(
                ImageBindFlags.TransferWrite | ImageBindFlags.ShaderRead, imageAsset.Width, imageAsset.Height,
                Format.R8G8B8A8_SRGB));
            textureImage.AllocateMemory(MemoryType.DeviceLocal);

            using (var transferComplete = device.CreateFence(Fence.FenceState.Reset))
            {
                using var vertexStagingBuffer = meshAsset.CreateVertexStagingBuffer(device);
                using var indexStagingBuffer = meshAsset.CreateIndexStagingBuffer(device);
                using var textureStagingBuffer = imageAsset.CreateStagingBuffer(device);
                using var commandBuffer = device.CreateCommandBuffer(CommandQueueClass.Transfer);
                using var transferQueue = device.GetCommandQueue(CommandQueueClass.Transfer);

                using (var builder = commandBuffer.Begin())
                {
                    builder.CopyBuffers(vertexStagingBuffer, vertexBuffer, meshAsset.VertexSize);
                    builder.CopyBuffers(indexStagingBuffer, indexBuffer, meshAsset.IndexSize);
                    builder.TransitionImageState(textureImage, ResourceState.TransferWrite);
                    builder.CopyBufferToImage(textureStagingBuffer, textureImage, imageAsset.ImageSize);
                    builder.TransitionImageState(textureImage, ResourceState.ShaderResource);
                }

                transferQueue.SubmitBuffers(commandBuffer, transferComplete, CommandQueue.SubmitFlags.None);
                transferComplete.WaitOnCpu();
            }

            textureSampler = device.CreateSampler(Sampler.Desc.Default);

            var compiler = device.CreateShaderCompiler();
            var vsArgs = ShaderCompiler.Args.FromFile(ShaderStage.Vertex, "../../Assets/Shaders/VertexShader.hlsl");
            using var vsBytecode = compiler.CompileShader(vsArgs);
            var psArgs = ShaderCompiler.Args.FromFile(ShaderStage.Pixel, "../../Assets/Shaders/PixelShader.hlsl");
            using var psBytecode = compiler.CompileShader(psArgs);

            pixelShader = device.CreateShaderModule(ShaderStage.Pixel, psBytecode);
            vertexShader = device.CreateShaderModule(ShaderStage.Vertex, vsBytecode);
            compiler.Dispose();

            var attachmentDesc = new AttachmentDesc(swapChain.Format, ResourceState.Undefined, ResourceState.Present);
            var depthAttachmentDesc = new AttachmentDesc(swapChain.DepthStencilView.Format, ResourceState.Undefined,
                ResourceState.DepthWrite);
            var subpassDesc = new SubpassDesc()
                .WithRenderTargetAttachments(new SubpassAttachment(ResourceState.RenderTarget, 0))
                .WithDepthStencilAttachment(new SubpassAttachment(ResourceState.DepthWrite, 1));
            var renderPassDesc = new RenderPass.Desc()
                .WithAttachments(attachmentDesc, depthAttachmentDesc)
                .WithSubpasses(subpassDesc)
                .WithSubpassDependencies(SubpassDependency.Default);
            renderPass = device.CreateRenderPass(renderPassDesc);

            var scissor = window.CreateScissor();
            var viewport = window.CreateViewport();

            descriptorAllocator = new DescriptorAllocator(device);
            descriptorTable = descriptorAllocator.Begin()
                .Bind(ShaderResourceType.Sampler, ShaderStageFlags.Pixel, 1)
                .Bind(ShaderResourceType.TextureSrv, ShaderStageFlags.Pixel, 1)
                .Bind(ShaderResourceType.ConstantBuffer, ShaderStageFlags.Vertex, 1)
                .End();

            descriptorTable.Update(0, textureSampler);
            descriptorTable.Update(1, textureImage.DefaultView);
            descriptorTable.Update(2, constantBuffer);

            var pipelineDesc = GraphicsPipeline.Desc.Default;
            pipelineDesc.InputLayout = new InputStreamLayout.Builder()
                .AddBuffer(InputStreamRate.PerVertex)
                .AddAttribute(Format.R32G32B32_SFloat, "POSITION")
                .AddAttribute(Format.R32G32_SFloat, "TEXCOORD")
                .Build()
                .Build();
            pipelineDesc.RenderPass = renderPass;
            pipelineDesc.SubpassIndex = 0;
            pipelineDesc.ColorBlend = new ColorBlendState(TargetColorBlending.Default);
            pipelineDesc.Shaders = new[] { pixelShader, vertexShader };
            pipelineDesc.DescriptorTables = new[] { descriptorTable };
            pipelineDesc.Rasterization = new RasterizationState(CullingModeFlags.None);
            pipelineDesc.Scissor = scissor;
            pipelineDesc.Viewport = viewport;
            pipelineDesc.DepthStencil = new DepthStencilState(true, true, CompareOp.Less);

            pipeline = device.CreateGraphicsPipeline(pipelineDesc);

            fences = new DisposableList<Fence>();
            for (var i = 0; i < swapChain.FrameCount; i++)
            {
                fences.Add(device.CreateFence(Fence.FenceState.Signaled));
            }

            framebuffers = new DisposableList<Framebuffer>();
            commandBuffers = new DisposableList<CommandBuffer>();
            for (var i = 0; i < swapChain.ImageCount; i++)
            {
                var framebufferDesc = new Framebuffer.Desc()
                    .WithRenderPass(renderPass)
                    .WithScissor(scissor)
                    .WithRenderTargetViews(swapChain.RenderTargetViews[i], swapChain.DepthStencilView);

                framebuffers.Add(device.CreateFramebuffer(framebufferDesc));
                commandBuffers.Add(device.CreateCommandBuffer(CommandQueueClass.Graphics));

                using var builder = commandBuffers[i].Begin();
                builder.BindGraphicsPipeline(pipeline);
                builder.BindDescriptorTables(pipeline, descriptorTable);
                builder.SetViewport(viewport);
                builder.SetScissor(scissor);
                builder.BindVertexBuffer(0, vertexBuffer);
                builder.BindIndexBuffer(indexBuffer);
                builder.BeginRenderPass(renderPass, framebuffers[i],
                    ClearValueDesc.CreateColorValue(Colors.MediumAquamarine),
                    ClearValueDesc.CreateDepthStencilValue());
                builder.DrawIndexed(meshAsset.IndexCount, 1, 0, 0, 0);
                builder.EndRenderPass();
            }

            commandQueue = device.GetCommandQueue(CommandQueueClass.Graphics);
        }

        private static Buffer CreateHostVisibleBuffer<T>(BindFlags bindFlags, Device device, T[] data)
            where T : unmanaged
        {
            var dataLength = (ulong)(data.Length * Marshal.SizeOf<T>());
            var stagingBuffer = device.CreateBuffer(bindFlags, dataLength);
            stagingBuffer.AllocateMemory(MemoryType.HostVisible);
            stagingBuffer.UpdateData(data);
            return stagingBuffer;
        }

        protected override void GetFrameworkDependencies(ICollection<IFrameworkFactory> dependencies)
        {
            dependencies.Add(new OsmiumGpuModule.Factory());
            dependencies.Add(new OsmiumAssetsModule.Factory());
        }

        protected override void PollSystemEvents()
        {
            window.PollEvents();
        }

        protected override void Tick(FrameEventArgs frameEventArgs)
        {
            var frameIndex = swapChain.CurrentFrameIndex;
            fences[frameIndex].WaitOnCpu();
            window.PollEvents();
            var imageIndex = swapChain.CurrentImageIndex;
            fences[swapChain.CurrentFrameIndex].Reset();
            commandQueue.SubmitBuffers(commandBuffers[imageIndex], fences[frameIndex], CommandQueue.SubmitFlags.FrameBeginEnd);
            swapChain.Present();
        }

        protected override void OnExit()
        {
            device.WaitIdle();
            this.DisposeFields();
        }
    }

    internal static class Program
    {
        private static int Main()
        {
            using var app = new ExampleApplication();
            app.Initialize(new ApplicationFramework.Desc(ExampleApplication.ApplicationName, "../../Assets"));
            return app.RunMainLoop();
        }
    }
}
