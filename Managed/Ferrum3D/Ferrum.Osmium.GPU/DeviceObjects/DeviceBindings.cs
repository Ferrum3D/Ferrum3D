using System;
using System.Runtime.InteropServices;
using Ferrum.Osmium.GPU.Descriptors;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public partial class Device
    {
        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateCommandBuffer")]
        private static extern IntPtr CreateCommandBufferNative(IntPtr self, CommandQueueClass cmdQueueClass);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateTransientResourceHeap")]
        private static extern IntPtr CreateTransientResourceHeapNative(IntPtr self, ref TransientResourceHeap.Desc desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateSampler")]
        private static extern IntPtr CreateSamplerNative(IntPtr self, ref Sampler.Desc desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateImage")]
        private static extern IntPtr CreateImageNative(IntPtr self, ref Image.Desc desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateDescriptorHeap")]
        private static extern IntPtr CreateDescriptorHeapNative(IntPtr self, ref DescriptorHeap.DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateFramebuffer")]
        private static extern IntPtr CreateFramebufferNative(IntPtr self, ref Framebuffer.DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateFence")]
        private static extern IntPtr CreateFenceNative(IntPtr self, Fence.FenceState state);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateGraphicsPipeline")]
        private static extern IntPtr CreateGraphicsPipelineNative(IntPtr self, ref GraphicsPipeline.DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateRenderPass")]
        private static extern IntPtr CreateRenderPassNative(IntPtr self, ref RenderPass.DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateShaderModule")]
        private static extern IntPtr CreateShaderModuleNative(IntPtr self, ref ShaderModule.DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateBuffer")]
        private static extern IntPtr CreateBufferNative(IntPtr self, ref Buffer.Desc desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateSwapChain")]
        private static extern IntPtr CreateSwapChainNative(IntPtr self, ref SwapChain.DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateShaderCompiler")]
        private static extern IntPtr CreateShaderCompilerNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_CreateWindow")]
        private static extern IntPtr CreateWindowNative(IntPtr self, ref Window.Desc desc);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_GetCommandQueue")]
        private static extern IntPtr GetCommandQueueNative(IntPtr self, int cmdQueueClass);

        [DllImport("OsGPUBindings", EntryPoint = "IDevice_WaitIdle")]
        private static extern void WaitIdleNative(IntPtr self);
    }
}
