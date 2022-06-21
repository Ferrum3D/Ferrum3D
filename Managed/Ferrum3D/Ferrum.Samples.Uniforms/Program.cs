using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Console;
using Ferrum.Core.Containers;
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

            var descriptorHeapDesc = new DescriptorHeap.Desc()
                .WithMaxTables(2)
                .WithSizes(
                    new DescriptorSize(1, ShaderResourceType.ConstantBuffer),
                    new DescriptorSize(1, ShaderResourceType.ConstantBuffer)
                );

            using var descriptorHeap = device.CreateDescriptorHeap(descriptorHeapDesc);
            var psDescriptorDesc = new DescriptorDesc(ShaderResourceType.ConstantBuffer, ShaderStageFlags.Pixel, 1);
            var vsDescriptorDesc = new DescriptorDesc(ShaderResourceType.ConstantBuffer, ShaderStageFlags.Vertex, 1);
            using var descriptorTable = descriptorHeap.AllocateDescriptorTable(psDescriptorDesc, vsDescriptorDesc);

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
                builder.BindDescriptorTables(new[] { descriptorTable }, pipeline);
                builder.SetViewport(viewport);
                builder.SetScissor(scissor);
                builder.BindVertexBuffer(0, vertexBuffer);
                builder.BindIndexBuffer(indexBuffer);
                builder.BeginRenderPass(renderPass, framebuffers[i], Colors.MediumAquamarine);
                builder.DrawIndexed(6, 1, 0, 0, 0);
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

            RunExample();
        }
    }
}
