using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Console;
using Ferrum.Core.Math;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.PipelineStates;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.VertexInput;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Samples.Uniforms
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct Vertex
    {
        public readonly float X;
        public readonly float Y;
        public readonly float Z;

        public Vertex(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }
    }

    internal static class Program
    {
        private static readonly Vertex[] vertexData =
        {
            new(-0.5f, -0.5f, 0.0f),
            new(+0.5f, +0.5f, 0.0f),
            new(+0.5f, -0.5f, 0.0f),
            new(-0.5f, +0.5f, 0.0f)
        };

        private static readonly uint[] indexData = { 0, 2, 3, 3, 2, 1 };

        private static readonly Color[] psConstantData = { Colors.Gold };
        private static readonly Vector3F[] vsConstantData = { new(0.3f, -0.4f, 0.0f) };

        private static Buffer CreateHostVisibleBuffer<T>(BindFlags bindFlags, Device device, T[] data)
            where T : unmanaged
        {
            var dataLength = (ulong)(data.Length * Marshal.SizeOf<T>());
            var stagingBuffer = device.CreateBuffer(bindFlags, dataLength);
            stagingBuffer.AllocateMemory(MemoryType.HostVisible);
            stagingBuffer.UpdateData(data);
            return stagingBuffer;
        }

        private static void RunExample()
        {
            var instanceDesc = new Instance.Desc("Ferrum3D - Uniforms");
            using var instance = new Instance(Engine.Environment, instanceDesc, GraphicsApi.Vulkan);
            using var adapter = instance.Adapters.First();
            using var device = adapter.CreateDevice();

            using var graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, "Ferrum3D - Uniforms");
            using var window = device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(window, graphicsQueue);
            using var swapChain = device.CreateSwapChain(swapChainDesc);

            using var psConstantBuffer = CreateHostVisibleBuffer(BindFlags.ConstantBuffer, device, psConstantData);
            using var vsConstantBuffer = CreateHostVisibleBuffer(BindFlags.ConstantBuffer, device, vsConstantData);

            var vertexSize = (ulong)(vertexData.Length * Marshal.SizeOf<Vertex>());
            var indexSize = (ulong)(indexData.Length * sizeof(uint));

            using var vertexBuffer = device.CreateBuffer(BindFlags.VertexBuffer, vertexSize);
            vertexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            using var indexBuffer = device.CreateBuffer(BindFlags.IndexBuffer, indexSize);
            indexBuffer.AllocateMemory(MemoryType.DeviceLocal);

            using (var transferComplete = device.CreateFence(Fence.FenceState.Reset))
            {
                using var vertexStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, vertexData);
                using var indexStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, indexData);
                using var commandBuffer = device.CreateCommandBuffer(CommandQueueClass.Transfer);
                using var transferQueue = device.GetCommandQueue(CommandQueueClass.Transfer);

                using (var builder = commandBuffer.Begin())
                {
                    builder.CopyBuffers(vertexStagingBuffer, vertexBuffer, new BufferCopyRegion(vertexSize));
                    builder.CopyBuffers(indexStagingBuffer, indexBuffer, new BufferCopyRegion(indexSize));
                }

                transferQueue.SubmitBuffers(commandBuffer, transferComplete, CommandQueue.SubmitFlags.None);
                transferComplete.WaitOnCpu();
            }

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
                .Build()
                .Build();
            pipelineDesc.RenderPass = renderPass;
            pipelineDesc.SubpassIndex = 0;
            pipelineDesc.ColorBlend = new ColorBlendState(TargetColorBlending.Default);
            pipelineDesc.Shaders = new[] { pixelShader, vertexShader };
            pipelineDesc.Rasterization = new RasterizationState(CullingModeFlags.Back);
            pipelineDesc.Scissor = scissor;
            pipelineDesc.Viewport = viewport;
        }

        private static void Main()
        {
            using var engine = new Engine();
            using var logger = new ConsoleLogger();

            RunExample();
        }
    }
}
