using System;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Console;
using Ferrum.Core.Containers;
using Ferrum.Core.Math;
using Ferrum.Core.Modules;
using Ferrum.Osmium.Assets;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.PipelineStates;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.VertexInput;
using Ferrum.Osmium.GPU.WindowSystem;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Samples.Textures
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct Vertex
    {
        public readonly float X;
        public readonly float Y;
        public readonly float Z;

        public readonly float U;
        public readonly float V;

        public Vertex(float x, float y, float z, float u, float v)
        {
            X = x;
            Y = y;
            Z = z;

            U = u;
            V = v;
        }
    }

    internal static class Program
    {
        private static readonly Vertex[] vertexData =
        {
            new(-0.5f, -0.5f, 0.0f, 1.0f, 0.0f),
            new(+0.5f, +0.5f, 0.0f, 0.0f, 1.0f),
            new(+0.5f, -0.5f, 0.0f, 0.0f, 0.0f),
            new(-0.5f, +0.5f, 0.0f, 1.0f, 1.0f)
        };

        private static readonly uint[] indexData = { 0, 2, 3, 3, 2, 1 };

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

        private static Buffer CreateHostVisibleBuffer(BindFlags bindFlags, Device device, ImageAsset data)
        {
            var stagingBuffer = device.CreateBuffer(bindFlags, data.ByteSize);
            stagingBuffer.AllocateMemory(MemoryType.HostVisible);
            stagingBuffer.UpdateData(data.DataHandle);
            return stagingBuffer;
        }

        private static void RunExample()
        {
            using var imageAsset = Asset.Load<ImageAsset>(Guid.Parse("94FC6391-4656-4BE7-844D-8D87680A00F1"));
            var instanceDesc = new Instance.Desc("Ferrum3D - Textures");
            using var instance = new Instance(instanceDesc, GraphicsApi.Vulkan);
            using var adapter = instance.Adapters.First();
            using var device = adapter.CreateDevice();

            using var graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, "Ferrum3D - Textures");
            using var window = device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(window, graphicsQueue);
            using var swapChain = device.CreateSwapChain(swapChainDesc);

            using var vsConstantBuffer = CreateHostVisibleBuffer(BindFlags.ConstantBuffer, device, vsConstantData);

            var vertexSize = (ulong)(vertexData.Length * Marshal.SizeOf<Vertex>());
            var indexSize = (ulong)(indexData.Length * sizeof(uint));

            using var vertexBuffer = device.CreateBuffer(BindFlags.VertexBuffer, vertexSize);
            vertexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            using var indexBuffer = device.CreateBuffer(BindFlags.IndexBuffer, indexSize);
            indexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            using var textureImage = device.CreateImage(Image.Desc.Img2D(
                ImageBindFlags.TransferWrite | ImageBindFlags.ShaderRead, imageAsset.Width, imageAsset.Height,
                Format.R8G8B8A8_SRGB));
            textureImage.AllocateMemory(MemoryType.DeviceLocal);

            using (var transferComplete = device.CreateFence(Fence.FenceState.Reset))
            {
                using var vertexStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, vertexData);
                using var indexStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, indexData);
                using var textureStagingBuffer = CreateHostVisibleBuffer(BindFlags.None, device, imageAsset);
                using var commandBuffer = device.CreateCommandBuffer(CommandQueueClass.Transfer);
                using var transferQueue = device.GetCommandQueue(CommandQueueClass.Transfer);

                using (var builder = commandBuffer.Begin())
                {
                    builder.CopyBuffers(vertexStagingBuffer, vertexBuffer, vertexSize);
                    builder.CopyBuffers(indexStagingBuffer, indexBuffer, indexSize);
                    builder.ResourceTransitionBarrier(textureImage, ResourceState.TransferWrite);
                    builder.CopyBufferToImage(textureStagingBuffer, textureImage, imageAsset.ImageSize);
                    builder.ResourceTransitionBarrier(textureImage, ResourceState.ShaderResource);
                }

                transferQueue.SubmitBuffers(commandBuffer, transferComplete, CommandQueue.SubmitFlags.None);
                transferComplete.WaitOnCpu();
            }

            using var textureSampler = device.CreateSampler(Sampler.Desc.Default);

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
                    new DescriptorSize(1, ShaderResourceType.Sampler),
                    new DescriptorSize(1, ShaderResourceType.TextureSrv),
                    new DescriptorSize(1, ShaderResourceType.ConstantBuffer)
                );

            using var descriptorHeap = device.CreateDescriptorHeap(descriptorHeapDesc);
            var descriptorSamplerDesc = new DescriptorDesc(ShaderResourceType.Sampler, ShaderStageFlags.Pixel, 1);
            var descriptorImageDesc = new DescriptorDesc(ShaderResourceType.TextureSrv, ShaderStageFlags.Pixel, 1);
            var vsDescriptorDesc = new DescriptorDesc(ShaderResourceType.ConstantBuffer, ShaderStageFlags.Vertex, 1);
            using var descriptorTable =
                descriptorHeap.AllocateDescriptorTable(descriptorSamplerDesc, descriptorImageDesc, vsDescriptorDesc);

            descriptorTable.Update(0, textureSampler);
            descriptorTable.Update(1, textureImage.DefaultView);
            descriptorTable.Update(2, vsConstantBuffer);

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
                    .WithRenderTargetViews(null, swapChain.RenderTargetViews[i]);

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
            using var assetManager = new AssetManager("Assets/FerrumAssetIndex");
            using var osmiumGpuModule = new OsmiumGpuModule(Engine.Environment);
            using var osmiumAssetsModule = new OsmiumAssetsModule(Engine.Environment);

            RunExample();
        }
    }
}
