using System;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public partial class Device : UnmanagedObject
    {
        public Device(IntPtr handle) : base(handle)
        {
        }

        public CommandBuffer CreateCommandBuffer(CommandQueueClass cmdQueueClass)
        {
            return new CommandBuffer(CreateCommandBufferNative(Handle, cmdQueueClass));
        }

        public Sampler CreateSampler(Sampler.Desc desc)
        {
            return new Sampler(CreateSamplerNative(Handle, ref desc));
        }

        public Image CreateImage(Image.Desc desc)
        {
            return new Image(CreateImageNative(Handle, ref desc));
        }

        public DescriptorHeap CreateDescriptorHeap(DescriptorHeap.Desc desc)
        {
            var nativeDesc = new DescriptorHeap.DescNative(desc);
            return new DescriptorHeap(CreateDescriptorHeapNative(Handle, ref nativeDesc));
        }

        public Framebuffer CreateFramebuffer(Framebuffer.Desc desc)
        {
            var nativeDesc = new Framebuffer.DescNative(desc);
            return new Framebuffer(CreateFramebufferNative(Handle, ref nativeDesc));
        }

        public Fence CreateFence(Fence.FenceState state)
        {
            return new Fence(CreateFenceNative(Handle, state));
        }

        public CommandQueue GetCommandQueue(CommandQueueClass cmdQueueClass)
        {
            return new CommandQueue(GetCommandQueueNative(Handle, (int)cmdQueueClass));
        }

        public GraphicsPipeline CreateGraphicsPipeline(GraphicsPipeline.Desc desc)
        {
            var nativeDesc = new GraphicsPipeline.DescNative(desc);
            return new GraphicsPipeline(CreateGraphicsPipelineNative(Handle, ref nativeDesc));
        }

        public RenderPass CreateRenderPass(RenderPass.Desc desc)
        {
            var nativeDesc = new RenderPass.DescNative(desc);
            return new RenderPass(CreateRenderPassNative(Handle, ref nativeDesc));
        }

        public SwapChain CreateSwapChain(SwapChain.Desc desc)
        {
            var nativeDesc = new SwapChain.DescNative(desc);
            return new SwapChain(CreateSwapChainNative(Handle, ref nativeDesc));
        }

        public ShaderModule CreateShaderModule(ShaderStage stage, NativeArray<byte> bytecode)
        {
            return CreateShaderModule(new ShaderModule.Desc(stage, bytecode));
        }

        public ShaderModule CreateShaderModule(ShaderModule.Desc desc)
        {
            var nativeDesc = new ShaderModule.DescNative(desc);
            return new ShaderModule(CreateShaderModuleNative(Handle, ref nativeDesc));
        }

        public Buffer CreateBuffer(BindFlags flags, ulong size)
        {
            return new Buffer(CreateBufferNative(Handle, (int)flags, size));
        }

        public Buffer CreateBuffer(BindFlags flags, int size)
        {
            return CreateBuffer(flags, (ulong)size);
        }

        public ShaderCompiler CreateShaderCompiler()
        {
            return new ShaderCompiler(CreateShaderCompilerNative(Handle));
        }

        public Window CreateWindow(Window.Desc desc)
        {
            return new Window(CreateWindowNative(Handle, ref desc), desc);
        }

        public void WaitIdle()
        {
            WaitIdleNative(Handle);
        }

        protected override void ReleaseUnmanagedResources()
        {
            WaitIdleNative(Handle);
            DestructNative(Handle);
        }
    }
}
