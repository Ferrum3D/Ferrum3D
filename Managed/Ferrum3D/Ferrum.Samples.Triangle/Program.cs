using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Console;
using Ferrum.Core.Containers;
using Ferrum.Core.Math;
using Ferrum.Core.Modules;
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
        public readonly float X;
        public readonly float Y;
        public readonly float Z;

        public readonly float R;
        public readonly float G;
        public readonly float B;

        public Vertex(float x, float y, float z, float r, float g, float b)
        {
            X = x;
            Y = y;
            Z = z;
            R = r;
            G = g;
            B = b;
        }
    }

    internal static class Program
    {
        private static readonly Vertex[] vertexData =
        {
            new(+0.0f, -0.5f, 0f, 1, 0, 0),
            new(+0.5f, +0.5f, 0f, 0, 1, 0),
            new(-0.5f, +0.5f, 0f, 0, 0, 1)
        };

        private static void RunExample()
        {
            var instanceDesc = new Instance.Desc("Ferrum3D - Triangle");
            using var instance = new Instance(instanceDesc, GraphicsApi.Vulkan);
            using var adapter = instance.Adapters.First();
            using var device = adapter.CreateDevice();

            using var graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, "Ferrum3D - Triangle");
            using var window = device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(window, graphicsQueue);
            using var swapChain = device.CreateSwapChain(swapChainDesc);

            var vertexDataLength = vertexData.Length * Marshal.SizeOf(typeof(Vertex));
            using var vertexBuffer = device.CreateBuffer(BindFlags.VertexBuffer, vertexDataLength);
            vertexBuffer.AllocateMemory(MemoryType.HostVisible);
            vertexBuffer.UpdateData(vertexData);

            var compiler = device.CreateShaderCompiler();
            var vsArgs = ShaderCompiler.Args.FromFile(ShaderStage.Vertex, "Assets/Shaders/VertexShader.hlsl");
            using var vsBytecode = compiler.CompileShader(vsArgs);
            var psArgs = ShaderCompiler.Args.FromFile(ShaderStage.Pixel, "Assets/Shaders/PixelShader.hlsl");
            using var psBytecode = compiler.CompileShader(psArgs);

            using var pixelShader = device.CreateShaderModule(ShaderStage.Pixel, psBytecode);
            using var vertexShader = device.CreateShaderModule(ShaderStage.Vertex, vsBytecode);
            compiler.Dispose();

            var attachmentDesc = new AttachmentDesc(swapChain.Format, ResourceState.Undefined, ResourceState.Present);
            var subpassDesc = new SubpassDesc()
                .WithRenderTargetAttachments(new SubpassAttachment(ResourceState.RenderTarget, 0));
            var renderPassDesc = new RenderPass.Desc()
                .WithAttachments(attachmentDesc)
                .WithSubpasses(subpassDesc)
                .WithSubpassDependencies(SubpassDependency.Default);
            using var renderPass = device.CreateRenderPass(renderPassDesc);

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

            using var pipeline = device.CreateGraphicsPipeline(pipelineDesc);

            using var fences = new DisposableList<Fence>();
            for (var i = 0; i < swapChain.FrameCount; i++)
            {
                fences.Add(device.CreateFence(Fence.FenceState.Signaled));
            }

            using var framebuffers = new DisposableList<Framebuffer>();
            using var commandBuffers = new DisposableList<CommandBuffer>();
            for (var i = 0; i < swapChain.ImageCount; i++)
            {
                var desc = new Framebuffer.Desc()
                    .WithRenderPass(renderPass)
                    .WithScissor(scissor)
                    .WithRenderTargetViews(swapChain.RenderTargetViews[i]);

                framebuffers.Add(device.CreateFramebuffer(desc));
                commandBuffers.Add(device.CreateCommandBuffer(CommandQueueClass.Graphics));

                using var builder = commandBuffers[i].Begin();
                builder.BindGraphicsPipeline(pipeline);
                builder.SetViewport(viewport);
                builder.SetScissor(scissor);
                builder.BindVertexBuffer(0, vertexBuffer);
                builder.BeginRenderPass(renderPass, framebuffers[i], Colors.MediumAquamarine);
                builder.Draw(6, 1, 0, 0);
                builder.EndRenderPass();
            }

            using var commandQueue = device.GetCommandQueue(CommandQueueClass.Graphics);
            while (!window.CloseRequested)
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

            device.WaitIdle();
        }

        private static void Main()
        {
            using var engine = new Engine();
            using var logger = new ConsoleLogger();
            OsmiumGpuModule.AttachEnvironment(Engine.Environment);

            ConsoleLogger.LogMessage("Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘");

            RunExample();
        }
    }
}
