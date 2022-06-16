using System;
using System.Runtime.InteropServices;
using Ferrum.Osmium.GPU.Shaders;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Device : DeviceObject
    {
        public Device(IntPtr handle) : base(handle)
        {
        }

        public CommandQueue GetCommandQueue(CommandQueueClass cmdQueueClass)
        {
            return new CommandQueue(GetCommandQueueNative(Handle, (int)cmdQueueClass));
        }

        public SwapChain CreateSwapChain(SwapChain.Desc desc)
        {
            var nativeDesc = new SwapChain.DescNative(desc);
            return new SwapChain(CreateSwapChainNative(Handle, ref nativeDesc));
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

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_CreateBuffer")]
        private static extern IntPtr CreateBufferNative(IntPtr self, int bindFlags, ulong size);

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_CreateSwapChain")]
        private static extern IntPtr CreateSwapChainNative(IntPtr self, ref SwapChain.DescNative desc);

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_CreateShaderCompiler")]
        private static extern IntPtr CreateShaderCompilerNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_CreateWindow")]
        private static extern IntPtr CreateWindowNative(IntPtr self, ref Window.Desc desc);

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IDevice_GetCommandQueue")]
        private static extern IntPtr GetCommandQueueNative(IntPtr self, int cmdQueueClass);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
