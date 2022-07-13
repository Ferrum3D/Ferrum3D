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
        internal RenderPass(IntPtr handle) : base(handle)
        {
        }

        [DllImport("OsGPUBindings", EntryPoint = "IRenderPass_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct DescNative
        {
            public readonly IntPtr Subpasses;
            public readonly IntPtr Attachments;
            public readonly IntPtr SubpassDependencies;

            public DescNative(in Desc desc)
            {
                var subpasses = desc.Subpasses.Select(x => new SubpassDesc.Native(x)).ToList();
                Subpasses = new NativeArray<SubpassDesc.Native>(subpasses).Detach();
                Attachments = new NativeArray<AttachmentDesc>(desc.Attachments).Detach();
                SubpassDependencies = new NativeArray<SubpassDependency>(desc.SubpassDependencies).Detach();
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
