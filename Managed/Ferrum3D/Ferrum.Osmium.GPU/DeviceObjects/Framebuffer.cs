using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.Common;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Framebuffer : UnmanagedObject
    {
        internal Framebuffer(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsGPUBindings", EntryPoint = "IFramebuffer_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct DescNative
        {
            public readonly IntPtr RenderTargetViews;
            public readonly IntPtr DepthStencilView;
            public readonly IntPtr RenderPass;
            public readonly uint Width;
            public readonly uint Height;

            public DescNative(Desc desc)
            {
                RenderTargetViews = NativeArray<IntPtr>.FromObjectCollection(desc.RenderTargetViews).Detach();
                DepthStencilView = desc.DepthStencilView?.Handle ?? IntPtr.Zero;
                RenderPass = desc.RenderPass.Handle;
                Width = (uint)desc.Width;
                Height = (uint)desc.Height;
            }
        }

        public readonly struct Desc
        {
            public readonly List<ImageView> RenderTargetViews;
            public readonly ImageView DepthStencilView;
            public readonly RenderPass RenderPass;
            public readonly int Width;
            public readonly int Height;

            private Desc(List<ImageView> renderTargetViews, ImageView depthStencilView, RenderPass renderPass,
                int width, int height)
            {
                RenderTargetViews = renderTargetViews;
                DepthStencilView = depthStencilView;
                RenderPass = renderPass;
                Width = width;
                Height = height;
            }

            public Desc WithRenderTargetViews(ImageView depthStencilView, params ImageView[] renderTargetViews)
            {
                return new Desc(renderTargetViews.ToList(), depthStencilView, RenderPass, Width, Height);
            }

            public Desc WithRenderPass(RenderPass renderPass)
            {
                return new Desc(RenderTargetViews, DepthStencilView, renderPass, Width, Height);
            }

            public Desc WithScissor(Scissor scissor)
            {
                return new Desc(RenderTargetViews, DepthStencilView, RenderPass, scissor.Width, scissor.Height);
            }
        }
    }
}
