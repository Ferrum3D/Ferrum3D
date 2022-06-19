using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.Common;

namespace Ferrum.Osmium.GPU.WindowSystem
{
    public class Window : UnmanagedObject
    {
        public uint Width => desc.Width;
        public uint Height => desc.Height;
        public string Title => desc.Title;
        public IntPtr NativeHandle { get; }

        public bool CloseRequested { get; private set; }

        private readonly Desc desc;

        public Window(IntPtr handle, Desc desc) : base(handle)
        {
            this.desc = desc;
            NativeHandle = GetNativeHandleNative(Handle);
        }

        public void PollEvents()
        {
            PollEventsNative(Handle);
            CloseRequested = CloseRequestedNative(Handle);
        }

        public Scissor CreateScissor()
        {
            CreateScissorNative(Handle, out var scissor);
            return scissor;
        }

        public Viewport CreateViewport()
        {
            CreateViewportNative(Handle, out var viewport);
            return viewport;
        }

        [DllImport("OsmiumBindings", EntryPoint = "IWindow_GetNativeHandle")]
        private static extern IntPtr GetNativeHandleNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IWindow_CreateScissor")]
        private static extern void CreateScissorNative(IntPtr self, out Scissor scissor);

        [DllImport("OsmiumBindings", EntryPoint = "IWindow_CreateViewport")]
        private static extern void CreateViewportNative(IntPtr self, out Viewport viewport);

        [DllImport("OsmiumBindings", EntryPoint = "IWindow_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IWindow_PollEvents")]
        private static extern void PollEventsNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "IWindow_CloseRequested")]
        private static extern bool CloseRequestedNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct Desc
        {
            public readonly uint Width;
            public readonly uint Height;
            public readonly string Title;

            public Desc(uint width, uint height, string title)
            {
                Width = width;
                Height = height;
                Title = title;
            }
        }
    }
}
