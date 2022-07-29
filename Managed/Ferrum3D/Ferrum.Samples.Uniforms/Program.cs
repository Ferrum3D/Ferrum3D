using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.EventBus;
using Ferrum.Core.Framework;
using Ferrum.Core.Math;
using Ferrum.Core.Utils;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.Descriptors;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.PipelineStates;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.VertexInput;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Samples.Uniforms
{
    internal class ExampleApplication : ApplicationFramework
    {
        public const string ApplicationName = "Ferrum3D - Uniforms";

        private static readonly Vector3F[] vertexData =
        {
            new(-0.5f, -0.5f, 0.0f),
            new(+0.5f, +0.5f, 0.0f),
            new(+0.5f, -0.5f, 0.0f),
            new(-0.5f, +0.5f, 0.0f)
        };

        private static readonly uint[] indexData = { 0, 2, 3, 3, 2, 1 };

        private static readonly Color[] psConstantData = { Colors.Gold };
        private static readonly Vector3F[] vsConstantData = { new(0.3f, -0.4f, 0.0f) };

        private Instance instance;
        private Adapter adapter;
        private Device device;
        private CommandQueue graphicsQueue;
        private Window window;
        private SwapChain swapChain;
        private Buffer psConstantBuffer;
        private Buffer vsConstantBuffer;
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
        private DescriptorHeap descriptorHeap;
        private DescriptorTable descriptorTable;

        protected override bool CloseEventReceived => window.CloseRequested;

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            var gpuModule = GetDependency<OsmiumGpuModule>();
            gpuModule.Initialize(new OsmiumGpuModule.Desc(ApplicationName, GraphicsApi.Vulkan));

            instance = gpuModule.CreateInstance();
            adapter = instance.Adapters.First();
            device = adapter.CreateDevice();

            graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, "Ferrum3D - Uniforms");
            window = device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(window, graphicsQueue);
            swapChain = device.CreateSwapChain(swapChainDesc);

            psConstantBuffer = CreateHostVisibleBuffer(BindFlags.ConstantBuffer, device, psConstantData);
            vsConstantBuffer = CreateHostVisibleBuffer(BindFlags.ConstantBuffer, device, vsConstantData);

            var vertexSize = (ulong)(vertexData.Length * Marshal.SizeOf<Vector3F>());
            var indexSize = (ulong)(indexData.Length * sizeof(uint));

            vertexBuffer = device.CreateBuffer(BindFlags.VertexBuffer, vertexSize);
            vertexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            indexBuffer = device.CreateBuffer(BindFlags.IndexBuffer, indexSize);
            indexBuffer.AllocateMemory(MemoryType.DeviceLocal);

            using (var transferComplete = device.CreateFence(Fence.FenceState.Reset))
            {
                using var vertexStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, vertexData);
                using var indexStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, indexData);
                using var commandBuffer = device.CreateCommandBuffer(CommandQueueClass.Transfer);
                using var transferQueue = device.GetCommandQueue(CommandQueueClass.Transfer);

                using (var builder = commandBuffer.Begin())
                {
                    builder.CopyBuffers(vertexStagingBuffer, vertexBuffer, vertexSize);
                    builder.CopyBuffers(indexStagingBuffer, indexBuffer, indexSize);
                }

                transferQueue.SubmitBuffers(commandBuffer, transferComplete, CommandQueue.SubmitFlags.None);
                transferComplete.WaitOnCpu();
            }

            var compiler = device.CreateShaderCompiler();
            var vsArgs = ShaderCompiler.Args.FromFile(ShaderStage.Vertex, "Assets/Shaders/VertexShader.hlsl");
            using var vsBytecode = compiler.CompileShader(vsArgs);
            var psArgs = ShaderCompiler.Args.FromFile(ShaderStage.Pixel, "Assets/Shaders/PixelShader.hlsl");
            using var psBytecode = compiler.CompileShader(psArgs);

            pixelShader = device.CreateShaderModule(ShaderStage.Pixel, psBytecode);
            vertexShader = device.CreateShaderModule(ShaderStage.Vertex, vsBytecode);
            compiler.Dispose();

            var attachmentDesc = new AttachmentDesc(swapChain.Format, ResourceState.Undefined, ResourceState.Present);
            var subpassDesc = new SubpassDesc()
                .WithRenderTargetAttachments(new SubpassAttachment(ResourceState.RenderTarget, 0));
            var renderPassDesc = new RenderPass.Desc()
                .WithAttachments(attachmentDesc)
                .WithSubpasses(subpassDesc)
                .WithSubpassDependencies(SubpassDependency.Default);
            renderPass = device.CreateRenderPass(renderPassDesc);

            var scissor = window.CreateScissor();
            var viewport = window.CreateViewport();

            var descriptorHeapDesc = new DescriptorHeap.Desc()
                .WithMaxTables(2)
                .WithSizes(
                    new DescriptorSize(1, ShaderResourceType.ConstantBuffer),
                    new DescriptorSize(1, ShaderResourceType.ConstantBuffer)
                );

            descriptorHeap = device.CreateDescriptorHeap(descriptorHeapDesc);
            var psDescriptorDesc = new DescriptorDesc(ShaderResourceType.ConstantBuffer, ShaderStageFlags.Pixel, 1);
            var vsDescriptorDesc = new DescriptorDesc(ShaderResourceType.ConstantBuffer, ShaderStageFlags.Vertex, 1);
            descriptorTable = descriptorHeap.AllocateDescriptorTable(psDescriptorDesc, vsDescriptorDesc);

            descriptorTable.Update(0, psConstantBuffer);
            descriptorTable.Update(1, vsConstantBuffer);

            var pipelineDesc = GraphicsPipeline.Desc.Default;
            pipelineDesc.InputLayout = new InputStreamLayout.Builder()
                .AddBuffer(InputStreamRate.PerVertex)
                .AddAttribute(Format.R32G32B32_SFloat, "POSITION")
                .Build()
                .Build();
            pipelineDesc.RenderPass = renderPass;
            pipelineDesc.SubpassIndex = 0;
            pipelineDesc.ColorBlend = new ColorBlendState(TargetColorBlending.Default);
            pipelineDesc.Shaders = new[] { pixelShader, vertexShader };
            pipelineDesc.DescriptorTables = new[] { descriptorTable };
            pipelineDesc.Rasterization = new RasterizationState(CullingModeFlags.Back);
            pipelineDesc.Scissor = scissor;
            pipelineDesc.Viewport = viewport;

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
                    .WithRenderTargetViews(swapChain.RenderTargetViews[i]);

                framebuffers.Add(device.CreateFramebuffer(framebufferDesc));
                commandBuffers.Add(device.CreateCommandBuffer(CommandQueueClass.Graphics));

                using var builder = commandBuffers[i].Begin();
                builder.BindGraphicsPipeline(pipeline);
                builder.BindDescriptorTables(pipeline, descriptorTable);
                builder.SetViewport(viewport);
                builder.SetScissor(scissor);
                builder.BindVertexBuffer(0, vertexBuffer);
                builder.BindIndexBuffer(indexBuffer);
                builder.BeginRenderPass(renderPass, framebuffers[i], Colors.MediumAquamarine);
                builder.DrawIndexed((uint)indexData.Length, 1, 0, 0, 0);
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
        }

        protected override void BeginFrame()
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
            commandQueue.SubmitBuffers(commandBuffers[imageIndex], fences[frameIndex],
                CommandQueue.SubmitFlags.FrameBeginEnd);
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
            app.Initialize(new ApplicationFramework.Desc(ExampleApplication.ApplicationName));
            return app.RunMainLoop();
        }
    }
}
