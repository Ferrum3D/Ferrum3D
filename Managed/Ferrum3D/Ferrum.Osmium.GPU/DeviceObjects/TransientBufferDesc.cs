using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct TransientBufferDesc
    {
        public readonly Buffer.Desc Descriptor;
        public readonly ulong ResourceID;

        public TransientBufferDesc(Buffer.Desc descriptor, ulong resourceId)
        {
            Descriptor = descriptor;
            ResourceID = resourceId;
        }
    }
}
