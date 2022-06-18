using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class RenderPass : UnmanagedObject
    {
        public RenderPass(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsmiumBindings", EntryPoint = "IRenderPass_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct SubpassDescNative
        {
            private readonly IntPtr InputAttachments;
            private readonly IntPtr RenderTargetAttachments;
            private readonly IntPtr PreserveAttachments;
            private readonly SubpassAttachment DepthStencilAttachment;

            public SubpassDescNative(SubpassDesc desc)
            {
                desc = desc.ReplaceNulls();
                InputAttachments = ByteBuffer.FromCollection(desc.InputAttachments).Detach();
                RenderTargetAttachments = ByteBuffer.FromCollection(desc.RenderTargetAttachments).Detach();
                PreserveAttachments = ByteBuffer.FromCollection(desc.PreserveAttachments).Detach();
                DepthStencilAttachment = desc.DepthStencilAttachment ?? SubpassAttachment.None;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct DescNative
        {
            public readonly IntPtr Subpasses;
            public readonly IntPtr Attachments;
            public readonly IntPtr SubpassDependencies;

            public DescNative(Desc desc)
            {
                var subpasses = desc.Subpasses.Select(x => new SubpassDescNative(x)).ToList();
                Subpasses = ByteBuffer.FromCollection(subpasses).Detach();
                Attachments = ByteBuffer.FromCollection(desc.Attachments).Detach();
                SubpassDependencies = ByteBuffer.FromCollection(desc.SubpassDependencies).Detach();
            }
        }

        public readonly struct Desc
        {
            public readonly List<SubpassDesc> Subpasses;
            public readonly List<AttachmentDesc> Attachments;
            public readonly List<SubpassDependency> SubpassDependencies;

            private Desc(List<SubpassDesc> subpasses, List<AttachmentDesc> attachments,
                List<SubpassDependency> subpassDependencies)
            {
                Subpasses = subpasses;
                Attachments = attachments;
                SubpassDependencies = subpassDependencies;
            }

            public Desc WithSubpasses(params SubpassDesc[] subpasses)
            {
                return new Desc(subpasses.ToList(), Attachments, SubpassDependencies);
            }

            public Desc WithAttachments(params AttachmentDesc[] attachments)
            {
                return new Desc(Subpasses, attachments.ToList(), SubpassDependencies);
            }

            public Desc WithSubpassDependencies(params SubpassDependency[] subpassDependencies)
            {
                return new Desc(Subpasses, Attachments, subpassDependencies.ToList());
            }
        }
    }
}
