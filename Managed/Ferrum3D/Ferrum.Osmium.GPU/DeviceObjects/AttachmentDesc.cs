using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct AttachmentDesc
    {
        public readonly AttachmentLoadOp LoadOp;
        public readonly AttachmentStoreOp StoreOp;
        public readonly AttachmentLoadOp StencilLoadOp;
        public readonly AttachmentStoreOp StencilStoreOp;
        public readonly Format Format;
        public readonly ResourceState InitialState;
        public readonly ResourceState FinalState;

        public AttachmentDesc(Format format, ResourceState initialState, ResourceState finalState)
        {
            Format = format;
            InitialState = initialState;
            FinalState = finalState;
            LoadOp = AttachmentLoadOp.Clear;
            StoreOp = AttachmentStoreOp.Store;
            StencilLoadOp = AttachmentLoadOp.DontCare;
            StencilStoreOp = AttachmentStoreOp.DontCare;
        }
    }
}
