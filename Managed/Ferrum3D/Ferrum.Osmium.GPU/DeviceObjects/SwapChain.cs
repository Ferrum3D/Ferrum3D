using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class SwapChain : UnmanagedObject
    {
        public SwapChain(IntPtr handle) : base(handle)
        {
        }

        private DescNative NativeDesc
        {
            get
            {
                GetDescNative(Handle, out var desc);
                return desc;
            }
        }
        
        public uint ImageCount => NativeDesc.ImageCount;
        public uint FrameCount => NativeDesc.FrameCount;
        public uint ImageWidth => NativeDesc.ImageWidth;
        public uint ImageHeight => NativeDesc.ImageHeight;
        public bool VerticalSync => NativeDesc.VerticalSync;
        public Format Format => (Format)NativeDesc.Format;

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_GetDesc")]
        private static extern void GetDescNative(IntPtr self, out DescNative desc);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Desc
        {
            public readonly uint ImageCount;
            public readonly uint FrameCount;
            public readonly uint ImageWidth;
            public readonly uint ImageHeight;
            public readonly bool VerticalSync;
            public readonly Format Format;
            public readonly CommandQueue Queue;
            public readonly Window Window;
            
            public Desc(Window window, CommandQueue queue, Format format = Format.None, bool verticalSync = false,
                uint imageCount = 3, uint frameCount = 2)
            {
                Window = window;
                Queue = queue;
                ImageCount = imageCount;
                FrameCount = frameCount;
                var scissor = Window.CreateScissor();
                ImageWidth = (uint)scissor.Width;
                ImageHeight = (uint)scissor.Height;
                Format = format;
                VerticalSync = verticalSync;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct DescNative
        {
            public readonly uint ImageCount;
            public readonly uint FrameCount;
            public readonly uint ImageWidth;
            public readonly uint ImageHeight;
            public readonly bool VerticalSync;
            public readonly int Format;
            public readonly IntPtr Queue;
            public readonly IntPtr NativeWindowHandle;

            public DescNative(Desc desc)
            {
                ImageCount = desc.ImageCount;
                FrameCount = desc.FrameCount;
                ImageWidth = desc.ImageWidth;
                ImageHeight = desc.ImageHeight;
                VerticalSync = desc.VerticalSync;
                Format = (int)desc.Format;
                Queue = desc.Queue.Handle;
                NativeWindowHandle = desc.Window.NativeHandle;
            }
        }
    }
}
