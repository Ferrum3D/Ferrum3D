using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.Common;
using Ferrum.Osmium.GPU.PipelineStates;
using Ferrum.Osmium.GPU.VertexInput;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class GraphicsPipeline : UnmanagedObject
    {
        public GraphicsPipeline(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsGPUBindings", EntryPoint = "IGraphicsPipeline_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct DescNative
        {
            public readonly ColorBlendState.Native ColorBlend;
            public readonly InputStreamLayout.Native InputLayout;
            public readonly RasterizationState.Native Rasterization;
            public readonly DepthStencilState.Native DepthStencil;
            public readonly Viewport Viewport;
            public readonly Scissor Scissor;
            public readonly IntPtr RenderPass;
            public readonly IntPtr DescriptorTables;
            public readonly IntPtr Shaders;
            public readonly uint SubpassIndex;

            public DescNative(Desc desc)
            {
                RenderPass = desc.RenderPass.Handle;
                SubpassIndex = desc.SubpassIndex;
                DescriptorTables = NativeArray<IntPtr>.FromObjectCollection(desc.DescriptorTables)?.Detach() ??
                                   IntPtr.Zero;
                Shaders = NativeArray<IntPtr>.FromObjectCollection(desc.Shaders)?.Detach() ?? IntPtr.Zero;
                Rasterization = new RasterizationState.Native(desc.Rasterization);
                DepthStencil = new DepthStencilState.Native(desc.DepthStencil);
                ColorBlend = new ColorBlendState.Native(desc.ColorBlend);
                InputLayout = new InputStreamLayout.Native(desc.InputLayout);
                Viewport = desc.Viewport;
                Scissor = desc.Scissor;
            }
        }

        public struct Desc
        {
            public static Desc Default = new Desc(null, 0, null, null, RasterizationState.Default,
                DepthStencilState.Default,
                new ColorBlendState(), new InputStreamLayout(), new Viewport(), new Scissor());

            public RenderPass RenderPass;
            public uint SubpassIndex;
            public ShaderModule[] Shaders;
            public DescriptorTable[] DescriptorTables;
            public RasterizationState Rasterization;
            public DepthStencilState DepthStencil;
            public ColorBlendState ColorBlend;
            public InputStreamLayout InputLayout;
            public Viewport Viewport;
            public Scissor Scissor;

            public Desc(RenderPass renderPass, uint subpassIndex, ShaderModule[] shaders,
                DescriptorTable[] descriptorTables,
                RasterizationState rasterization, DepthStencilState depthStencil, ColorBlendState colorBlend,
                InputStreamLayout inputLayout, Viewport viewport, Scissor scissor)
            {
                RenderPass = renderPass;
                SubpassIndex = subpassIndex;
                Shaders = shaders;
                DescriptorTables = descriptorTables;
                Rasterization = rasterization;
                DepthStencil = depthStencil;
                ColorBlend = colorBlend;
                InputLayout = inputLayout;
                Viewport = viewport;
                Scissor = scissor;
            }
        }
    }
}
