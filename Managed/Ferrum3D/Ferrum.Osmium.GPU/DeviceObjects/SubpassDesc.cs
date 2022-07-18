using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public readonly struct SubpassDesc
    {
        public readonly List<SubpassAttachment> InputAttachments;
        public readonly List<SubpassAttachment> RenderTargetAttachments;
        public readonly List<SubpassAttachment> MsaaResolveAttachments;
        public readonly List<int> PreserveAttachments;
        public readonly SubpassAttachment? DepthStencilAttachment;

        private SubpassDesc(List<SubpassAttachment> inputAttachments,
            List<SubpassAttachment> renderTargetAttachments, List<SubpassAttachment> msaaResolveAttachments,
            List<int> preserveAttachments, SubpassAttachment? depthStencilAttachment)
        {
            InputAttachments = inputAttachments;
            RenderTargetAttachments = renderTargetAttachments;
            MsaaResolveAttachments = msaaResolveAttachments;
            PreserveAttachments = preserveAttachments;
            DepthStencilAttachment = depthStencilAttachment;
        }

        public SubpassDesc WithDepthStencilAttachment(SubpassAttachment v)
        {
            return new SubpassDesc(InputAttachments, RenderTargetAttachments, MsaaResolveAttachments,
                PreserveAttachments, v);
        }

        public SubpassDesc WithInputAttachments(params SubpassAttachment[] v)
        {
            return new SubpassDesc(v.ToList(), RenderTargetAttachments, MsaaResolveAttachments, PreserveAttachments,
                DepthStencilAttachment);
        }

        public SubpassDesc WithRenderTargetAttachments(params SubpassAttachment[] v)
        {
            return new SubpassDesc(InputAttachments, v.ToList(), MsaaResolveAttachments, PreserveAttachments,
                DepthStencilAttachment);
        }

        public SubpassDesc WithMsaaResolveAttachments(params SubpassAttachment[] v)
        {
            return new SubpassDesc(InputAttachments, RenderTargetAttachments, v.ToList(), PreserveAttachments,
                DepthStencilAttachment);
        }

        public SubpassDesc WithPreserveAttachments(params int[] v)
        {
            return new SubpassDesc(InputAttachments, RenderTargetAttachments, MsaaResolveAttachments, v.ToList(),
                DepthStencilAttachment);
        }

        private SubpassDesc ReplaceNulls()
        {
            return new SubpassDesc(
                InputAttachments ?? new List<SubpassAttachment>(),
                RenderTargetAttachments ?? new List<SubpassAttachment>(),
                MsaaResolveAttachments ?? new List<SubpassAttachment>(),
                PreserveAttachments ?? new List<int>(),
                DepthStencilAttachment ?? SubpassAttachment.None
            );
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public readonly ByteBuffer.Native InputAttachments;
            public readonly ByteBuffer.Native RenderTargetAttachments;
            public readonly ByteBuffer.Native MSAAResolveAttachments;
            public readonly ByteBuffer.Native PreserveAttachments;
            public readonly SubpassAttachment DepthStencilAttachment;

            public Native(SubpassDesc desc)
            {
                desc = desc.ReplaceNulls();
                InputAttachments = new NativeArray<SubpassAttachment>(desc.InputAttachments).Detach();
                RenderTargetAttachments = new NativeArray<SubpassAttachment>(desc.RenderTargetAttachments).Detach();
                MSAAResolveAttachments = new NativeArray<SubpassAttachment>(desc.MsaaResolveAttachments).Detach();
                PreserveAttachments = new NativeArray<int>(desc.PreserveAttachments).Detach();
                DepthStencilAttachment = desc.DepthStencilAttachment ?? SubpassAttachment.None;
            }
        }
    }
}
