using System;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Console;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.Shaders;
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
            var instanceDesc = new Instance.Desc("TestApp");
            using var instance = new Instance(Engine.Environment, instanceDesc, GraphicsApi.Vulkan);
            using var adapter = instance.Adapters.First();
            using var device = adapter.CreateDevice();

            using var graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(800, 600, "TestApp");
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

            while (!window.CloseRequested)
            {
                window.PollEvents();
            }
        }

        private static void Main()
        {
            using var engine = new Engine();
            using var logger = new ConsoleLogger();

            ConsoleLogger.LogMessage("Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘");

            RunExample();
        }
    }
}
