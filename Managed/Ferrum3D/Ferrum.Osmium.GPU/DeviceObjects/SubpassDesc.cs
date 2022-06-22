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
        public readonly List<uint> PreserveAttachments;
        public readonly SubpassAttachment? DepthStencilAttachment;

        private SubpassDesc(List<SubpassAttachment> inputAttachments,
            List<SubpassAttachment> renderTargetAttachments,
            List<uint> preserveAttachments, SubpassAttachment? depthStencilAttachment)
        {
            InputAttachments = inputAttachments;
            RenderTargetAttachments = renderTargetAttachments;
            PreserveAttachments = preserveAttachments;
            DepthStencilAttachment = depthStencilAttachment;
        }

        public SubpassDesc WithDepthStencilAttachment(SubpassAttachment v)
        {
            return new SubpassDesc(InputAttachments, RenderTargetAttachments, PreserveAttachments, v);
        }

        public SubpassDesc WithInputAttachments(params SubpassAttachment[] v)
        {
            return new SubpassDesc(v.ToList(), RenderTargetAttachments, PreserveAttachments, DepthStencilAttachment);
        }

        public SubpassDesc WithRenderTargetAttachments(params SubpassAttachment[] v)
        {
            return new SubpassDesc(InputAttachments, v.ToList(), PreserveAttachments, DepthStencilAttachment);
        }

        public SubpassDesc WithPreserveAttachments(params uint[] v)
        {
            return new SubpassDesc(InputAttachments, RenderTargetAttachments, v.ToList(), DepthStencilAttachment);
        }

        internal SubpassDesc ReplaceNulls()
        {
            return new SubpassDesc(
                InputAttachments ?? new List<SubpassAttachment>(),
                RenderTargetAttachments ?? new List<SubpassAttachment>(),
                PreserveAttachments ?? new List<uint>(),
                DepthStencilAttachment ?? SubpassAttachment.None
            );
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public readonly IntPtr InputAttachments;
            public readonly IntPtr RenderTargetAttachments;
            public readonly IntPtr PreserveAttachments;
            public readonly SubpassAttachment DepthStencilAttachment;

            public Native(SubpassDesc desc)
            {
                desc = desc.ReplaceNulls();
                InputAttachments = new NativeArray<SubpassAttachment>(desc.InputAttachments).Detach();
                RenderTargetAttachments = new NativeArray<SubpassAttachment>(desc.RenderTargetAttachments).Detach();
                PreserveAttachments = new NativeArray<uint>(desc.PreserveAttachments).Detach();
                DepthStencilAttachment = desc.DepthStencilAttachment ?? SubpassAttachment.None;
            }
        }
    }
}
