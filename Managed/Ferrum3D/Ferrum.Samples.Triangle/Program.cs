using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.EventBus;
using Ferrum.Core.Framework;
using Ferrum.Core.Math;
using Ferrum.Core.Utils;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.PipelineStates;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.VertexInput;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Samples.Triangle
{
    [StructLayout(LayoutKind.Sequential)]
    internal readonly struct Vertex
    {
        public readonly Vector3F Position;
        public readonly Vector3F Color;

        public Vertex(float x, float y, float z, Color color)
        {
            Position = new Vector3F(x, y, z);
            Color = color.GetVector3();
        }
    }

    internal class ExampleApplication : ApplicationFramework
    {
        private static readonly Vertex[] vertexData =
        {
            new(+0.0f, -0.5f, 0f, Colors.Red),
            new(+0.5f, +0.5f, 0f, Colors.Green),
            new(-0.5f, +0.5f, 0f, Colors.Blue)
        };

        public const string ApplicationName = "Ferrum3D - Triangle";
        private Instance instance;
        private Adapter adapter;
        private Device device;
        private CommandQueue graphicsQueue;
        private Window window;
        private SwapChain swapChain;
        private Buffer vertexBuffer;
        private DisposableList<Fence> fences;
        private ShaderModule pixelShader;
        private ShaderModule vertexShader;
        private RenderPass renderPass;
        private GraphicsPipeline pipeline;
        private DisposableList<Framebuffer> framebuffers;
        private DisposableList<CommandBuffer> commandBuffers;
        private CommandQueue commandQueue;

        protected override void GetFrameworkDependencies(ICollection<IFrameworkFactory> dependencies)
        {
            dependencies.Add(new OsmiumGpuModule.Factory());
        }

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            var gpuModule = GetDependency<OsmiumGpuModule>();
            gpuModule.Initialize(new OsmiumGpuModule.Desc(ApplicationName, GraphicsApi.Vulkan));

            instance = gpuModule.CreateInstance();
            adapter = instance.Adapters.First();
            device = adapter.CreateDevice();

            graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, ApplicationName);
            window = device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(window, graphicsQueue);
            swapChain = device.CreateSwapChain(swapChainDesc);

            var vertexDataLength = vertexData.Length * Marshal.SizeOf(typeof(Vertex));
            vertexBuffer = device.CreateBuffer(BindFlags.VertexBuffer, vertexDataLength);
            vertexBuffer.AllocateMemory(MemoryType.HostVisible);
            vertexBuffer.UpdateData(vertexData);

            using var compiler = device.CreateShaderCompiler();
            var vsArgs = ShaderCompiler.Args.FromFile(ShaderStage.Vertex, "Assets/Shaders/VertexShader.hlsl");
            using var vsBytecode = compiler.CompileShader(vsArgs);
            var psArgs = ShaderCompiler.Args.FromFile(ShaderStage.Pixel, "Assets/Shaders/PixelShader.hlsl");
            using var psBytecode = compiler.CompileShader(psArgs);

            pixelShader = device.CreateShaderModule(ShaderStage.Pixel, psBytecode);
            vertexShader = device.CreateShaderModule(ShaderStage.Vertex, vsBytecode);

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

            var pipelineDesc = GraphicsPipeline.Desc.Default;
            pipelineDesc.InputLayout = new InputStreamLayout.Builder()
                .AddBuffer(InputStreamRate.PerVertex)
                .AddAttribute(Format.R32G32B32_SFloat, "POSITION")
                .AddAttribute(Format.R32G32B32_SFloat, "COLOR")
                .Build()
                .Build();
            pipelineDesc.RenderPass = renderPass;
            pipelineDesc.SubpassIndex = 0;
            pipelineDesc.ColorBlend = new ColorBlendState(TargetColorBlending.Default);
            pipelineDesc.Shaders = new[] { pixelShader, vertexShader };
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
                    .WithRenderTargetViews(null, swapChain.RenderTargetViews[i]);

                framebuffers.Add(device.CreateFramebuffer(framebufferDesc));
                commandBuffers.Add(device.CreateCommandBuffer(CommandQueueClass.Graphics));

                using var builder = commandBuffers[i].Begin();
                builder.BindGraphicsPipeline(pipeline);
                builder.SetViewport(viewport);
                builder.SetScissor(scissor);
                builder.BindVertexBuffer(0, vertexBuffer);
                builder.BeginRenderPass(renderPass, framebuffers[i], Colors.MediumAquamarine);
                builder.Draw(vertexData.Length, 1, 0, 0);
                builder.EndRenderPass();
            }

            commandQueue = device.GetCommandQueue(CommandQueueClass.Graphics);
        }

        protected override bool CloseEventReceived => window.CloseRequested;

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
