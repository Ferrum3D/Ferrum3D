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

        [DllImport("OsmiumBindings", EntryPoint = "IGraphicsPipeline_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct DescNative
        {
            public ColorBlendState.Native ColorBlend;
            public InputStreamLayout.Native InputLayout;
            public RasterizationState.Native Rasterization;
            public DepthStencilState.Native DepthStencil;
            public Viewport Viewport;
            public Scissor Scissor;
            public IntPtr RenderPass;
            public IntPtr DescriptorTables;
            public IntPtr Shaders;
            public uint SubpassIndex;

            public DescNative(Desc desc)
            {
                RenderPass = desc.RenderPass.Handle;
                SubpassIndex = desc.SubpassIndex;
                DescriptorTables = ByteBuffer.FromObjectCollection(desc.DescriptorTables).Detach();
                Shaders = ByteBuffer.FromObjectCollection(desc.Shaders).Detach();
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
            public static Desc Default = new Desc(null, 0, null, null, RasterizationState.Default, DepthStencilState.Default,
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

            public Desc(RenderPass renderPass, uint subpassIndex, ShaderModule[] shaders, DescriptorTable[] descriptorTables,
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
