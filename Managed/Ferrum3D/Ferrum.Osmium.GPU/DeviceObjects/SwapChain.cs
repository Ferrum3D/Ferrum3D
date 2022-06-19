using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class SwapChain : UnmanagedObject
    {
        public uint ImageCount => NativeDesc.ImageCount;
        public uint FrameCount => NativeDesc.FrameCount;
        public uint ImageWidth => NativeDesc.ImageWidth;
        public uint ImageHeight => NativeDesc.ImageHeight;
        public bool VerticalSync => NativeDesc.VerticalSync;
        public Format Format => (Format)NativeDesc.Format;

        public int CurrentFrameIndex => (int)GetCurrentFrameIndexNative(Handle);
        public int CurrentImageIndex => (int)GetCurrentImageIndexNative(Handle);

        public IReadOnlyList<ImageView> RenderTargetViews => renderTargetViews;

        private readonly ImageView[] renderTargetViews;

        private DescNative NativeDesc
        {
            get
            {
                GetDescNative(Handle, out var desc);
                return desc;
            }
        }

        public SwapChain(IntPtr handle) : base(handle)
        {
            GetRTVsNative(Handle, null, out var rtvCount);
            var rtv = new IntPtr[rtvCount];
            GetRTVsNative(Handle, rtv, out _);
            renderTargetViews = rtv.Select(x => new ImageView(x)).ToArray();
        }

        public void Present()
        {
            PresentNative(Handle);
        }

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_GetRTVs")]
        private static extern void GetRTVsNative(IntPtr self, IntPtr[] renderTargets, out uint count);
        
        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_Present")]
        private static extern void PresentNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_GetDesc")]
        private static extern void GetDescNative(IntPtr self, out DescNative desc);

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_GetCurrentFrameIndex")]
        private static extern uint GetCurrentFrameIndexNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ISwapChain_GetCurrentImageIndex")]
        private static extern uint GetCurrentImageIndexNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            foreach (var rtv in renderTargetViews)
            {
                rtv.Dispose();
            }

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
